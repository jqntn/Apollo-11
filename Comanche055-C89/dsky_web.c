#ifdef _WIN32
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#endif

/*
 * dsky_web.c -- Minimal HTTP/SSE web backend for DSKY
 *
 * Comanche055 (Apollo 11 CM) ANSI C89 port
 * Windows-first implementation using non-blocking Winsock.
 */

#include "dsky_web.h"

#ifdef _WIN32

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "dsky.h"

#define WEB_PORT                    8080
#define WEB_MAX_CLIENTS             16
#define WEB_RX_BUF                  2048
#define WEB_TX_BUF                  8192
#define WEB_SSE_FRAME_BUF           1024
#define WEB_MAX_BODY                128
#define WEB_REQ_LINE_BUF            256
#define WEB_PATH_BUF                128
#define WEB_HEADER_LINE_BUF         256
#define WEB_MAX_ACCEPTS_PER_TICK    4
#define WEB_STALL_TICKS_LIMIT       100
#define WEB_HEARTBEAT_TICKS         1500   /* 15s at 100Hz */
#define WEB_KEY_QUEUE_CAP           64

#define WEB_METHOD_OTHER            0
#define WEB_METHOD_GET              1
#define WEB_METHOD_POST             2

typedef struct {
    SOCKET sock;
    int active;
    int is_sse;
    int close_after_tx;
    int stalled_ticks;

    char rx_buf[WEB_RX_BUF];
    int rx_len;

    char tx_buf[WEB_TX_BUF];
    int tx_len;
    int tx_off;

    char sse_next_buf[WEB_SSE_FRAME_BUF];
    int sse_next_len;
} web_client_t;

typedef struct {
    int method;
    char path[WEB_PATH_BUF];
    int content_length;
    char body[WEB_MAX_BODY + 1];
} web_request_t;

static SOCKET web_listen_sock = INVALID_SOCKET;
static web_client_t web_clients[WEB_MAX_CLIENTS];
static int web_running = 0;
static dsky_display_t web_prev_display;
static int web_prev_display_valid = 0;
static int web_heartbeat_counter = 0;

static int web_key_queue[WEB_KEY_QUEUE_CAP];
static int web_key_head = 0;
static int web_key_tail = 0;
static int web_key_count = 0;

static const char web_index_html[] =
"<!doctype html>\n"
"<html lang='en'>\n"
"<head>\n"
"<meta charset='utf-8'>\n"
"<meta name='viewport' content='width=device-width,initial-scale=1'>\n"
"<title>Comanche055 DSKY</title>\n"
"<style>\n"
"html,body{height:100%;}\n"
"body{margin:0;padding:12px;background:#0b0b0b;color:#d4f7d4;font-family:Consolas,'Courier New',monospace;font-size:16px;line-height:1.45;display:flex;align-items:flex-start;justify-content:center;}\n"
".app{width:100%;max-width:760px;box-sizing:border-box;}\n"
"h1{margin:0 0 12px;text-align:center;font-size:30px;line-height:1.2;}\n"
"#status{margin:0 0 12px;text-align:center;color:#9ad09a;font-size:18px;}\n"
"pre{margin:0;background:#111;border:1px solid #2f2f2f;padding:16px;overflow:auto;font-size:22px;line-height:1.35;}\n"
".keys{margin:14px auto 0;display:flex;flex-wrap:wrap;gap:8px;justify-content:center;width:100%;max-width:560px;}\n"
"button{width:104px;min-height:50px;padding:10px 6px;background:#202020;border:1px solid #3a3a3a;color:#e8ffe8;cursor:pointer;font-family:Consolas,'Courier New',monospace;font-size:18px;}\n"
"button:active{background:#2a2a2a;}\n"
".spacer{pointer-events:none;cursor:default;}\n"
"</style>\n"
"</head>\n"
"<body>\n"
"<div class='app'>\n"
"<h1>COMANCHE 055 DSKY (Web)</h1>\n"
"<div id='status'>Connecting...</div>\n"
"<pre id='screen'>Waiting for state...</pre>\n"
"<div class='keys' id='keys'></div>\n"
"</div>\n"
"<script>\n"
"var KEY={\n"
" VERB:17,NOUN:31,PLUS:26,MINUS:27,ENTR:28,CLR:30,KREL:25,RSET:18,PRO:-1,\n"
" D0:16,D1:1,D2:2,D3:3,D4:4,D5:5,D6:6,D7:7,D8:8,D9:9\n"
"};\n"
"var CAN_FETCH=(typeof window.fetch==='function'&&window.JSON&&typeof JSON.stringify==='function');\n"
"var CAN_SSE=(typeof window.EventSource==='function');\n"
"var buttons=[\n"
" ['VERB',KEY.VERB],['NOUN',KEY.NOUN],['ENTR',KEY.ENTR],['CLR',KEY.CLR],['RSET',KEY.RSET],\n"
" ['+',KEY.PLUS],['-',KEY.MINUS],['PRO',KEY.PRO],['KREL',KEY.KREL],['',null],\n"
" ['7',KEY.D7],['8',KEY.D8],['9',KEY.D9],['4',KEY.D4],['5',KEY.D5],\n"
" ['6',KEY.D6],['1',KEY.D1],['2',KEY.D2],['3',KEY.D3],['0',KEY.D0]\n"
"];\n"
"function setStatus(t){document.getElementById('status').textContent=t;}\n"
"function d(n){return (n>=0&&n<=9)?String(n):' ';}\n"
"function sgn(n){return n>0?'+':(n<0?'-':' ');} \n"
"var LIGHT_ROWS=[\n"
" [['uplink_acty','UPLINK ACTY'],['temp','TEMP'],['prog_alarm','PROG']],\n"
" [['gimbal_lock','GIMBAL LOCK'],['stby','STBY'],['restart','RESTART']],\n"
" [['no_att','NO ATT'],['key_rel','KEY REL'],['tracker','TRACKER']],\n"
" [['opr_err','OPR ERR'],['vel','VEL'],['alt','ALT']]\n"
"];\n"
"function padRight(s,w){while(s.length<w)s+=' ';return s;}\n"
"function lightCell(on,label){return '['+(on?'X':' ')+'] '+label;}\n"
"function lightsBlock(l){\n"
" var rows=LIGHT_ROWS;\n"
" var i;\n"
" var j;\n"
" var out='LIGHTS:\\n';\n"
" var colw=18;\n"
" for(i=0;i<rows.length;i++){\n"
"  for(j=0;j<rows[i].length;j++){\n"
"   out+=padRight(lightCell(l[rows[i][j][0]]?1:0,rows[i][j][1]),colw);\n"
"  }\n"
"  out+='\\n';\n"
" }\n"
" out+=lightCell(l.comp_acty?1:0,'COMP ACTY')+'\\n';\n"
" return out;\n"
"}\n"
"function reg(r){return sgn(r.sign)+d(r.digits[0])+d(r.digits[1])+d(r.digits[2])+d(r.digits[3])+d(r.digits[4]);}\n"
"function render(st){\n"
" var out=lightsBlock(st.lights);\n"
" out+='PROG '+d(st.prog[0])+d(st.prog[1])+'  VERB '+d(st.verb[0])+d(st.verb[1])+'  NOUN '+d(st.noun[0])+d(st.noun[1])+'\\n';\n"
" out+='R1 '+reg(st.r1)+'\\n';\n"
" out+='R2 '+reg(st.r2)+'\\n';\n"
" out+='R3 '+reg(st.r3)+'\\n';\n"
" document.getElementById('screen').textContent=out;\n"
"}\n"
"function sendKey(code){\n"
" if(!CAN_FETCH){setStatus('Input unsupported: fetch unavailable');return;}\n"
" fetch('/key',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({keycode:code})}).catch(function(){});\n"
"}\n"
"function keycodeFromEvent(ev){\n"
" var k='';\n"
" var code=ev.which||ev.keyCode||0;\n"
" if(typeof ev.key==='string')k=ev.key;\n"
" if(k){\n"
"  if(k==='v'||k==='V')return KEY.VERB;\n"
"  if(k==='n'||k==='N')return KEY.NOUN;\n"
"  if(k==='e'||k==='E'||k==='Enter')return KEY.ENTR;\n"
"  if(k==='c'||k==='C')return KEY.CLR;\n"
"  if(k==='r'||k==='R')return KEY.RSET;\n"
"  if(k==='k'||k==='K')return KEY.KREL;\n"
"  if(k==='p'||k==='P')return KEY.PRO;\n"
"  if(k==='+'||k==='=')return KEY.PLUS;\n"
"  if(k==='-'||k==='_')return KEY.MINUS;\n"
"  if(k>='0'&&k<='9')return (k==='0')?KEY.D0:(k.charCodeAt(0)-48);\n"
" }\n"
" if(code>=48&&code<=57)return (code==48)?KEY.D0:(code-48);\n"
" if(code>=96&&code<=105)return (code==96)?KEY.D0:(code-96);\n"
" switch(code){\n"
"  case 13:return KEY.ENTR;\n"
"  case 67:return KEY.CLR;\n"
"  case 69:return KEY.ENTR;\n"
"  case 75:return KEY.KREL;\n"
"  case 78:return KEY.NOUN;\n"
"  case 80:return KEY.PRO;\n"
"  case 82:return KEY.RSET;\n"
"  case 86:return KEY.VERB;\n"
"  case 107:return KEY.PLUS;\n"
"  case 109:return KEY.MINUS;\n"
"  case 173:return KEY.MINUS;\n"
"  case 187:return KEY.PLUS;\n"
"  case 189:return KEY.MINUS;\n"
" }\n"
" return null;\n"
"}\n"
"(function(){\n"
" var root=document.getElementById('keys');\n"
" for(var i=0;i<buttons.length;i++){\n"
"  var b=document.createElement('button');\n"
"  if(buttons[i][1]===null){b.className='spacer';b.textContent=' ';b.tabIndex=-1;}\n"
"  else{b.textContent=buttons[i][0];(function(code){b.onclick=function(){sendKey(code);};})(buttons[i][1]);}\n"
"  root.appendChild(b);\n"
" }\n"
"})();\n"
"document.addEventListener('keydown',function(ev){\n"
" var kc=keycodeFromEvent(ev);\n"
" if(kc!==null){sendKey(kc);if(ev.preventDefault)ev.preventDefault();}\n"
"});\n"
"if(CAN_SSE){\n"
" var es=new EventSource('/events');\n"
" es.onopen=function(){setStatus(CAN_FETCH?'Connected':'Connected (read-only mode)');};\n"
" es.onmessage=function(ev){\n"
"  try{render(JSON.parse(ev.data));}\n"
"  catch(e){setStatus('Invalid state payload');}\n"
" };\n"
" es.onerror=function(){setStatus('Disconnected, retrying...');};\n"
"}else{\n"
" setStatus('Live updates unsupported: EventSource unavailable');\n"
"}\n"
"</script>\n"
"</body>\n"
"</html>\n";

static int web_is_space(char ch)
{
    return (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n');
}

static int web_is_digit(char ch)
{
    return (ch >= '0' && ch <= '9');
}

static int web_ascii_tolower(int ch)
{
    if (ch >= 'A' && ch <= 'Z') return ch - 'A' + 'a';
    return ch;
}

static int web_starts_with_ci(const char *str, const char *prefix)
{
    while (*prefix) {
        if (*str == '\0') return 0;
        if (web_ascii_tolower((unsigned char)*str) !=
            web_ascii_tolower((unsigned char)*prefix)) {
            return 0;
        }
        str++;
        prefix++;
    }
    return 1;
}

static const char *web_status_text(int status)
{
    switch (status) {
        case 200: return "OK";
        case 400: return "Bad Request";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 413: return "Payload Too Large";
        case 503: return "Service Unavailable";
        default:  return "Error";
    }
}

static int web_set_nonblocking(SOCKET sock)
{
    u_long mode;
    mode = 1;
    return ioctlsocket(sock, FIONBIO, &mode);
}

static void web_close_socket(SOCKET sock)
{
    if (sock != INVALID_SOCKET) {
        closesocket(sock);
    }
}

static void web_reset_client(web_client_t *c)
{
    memset(c, 0, sizeof(*c));
    c->sock = INVALID_SOCKET;
}

static int web_find_free_client_slot(void)
{
    int i;
    for (i = 0; i < WEB_MAX_CLIENTS; i++) {
        if (!web_clients[i].active) return i;
    }
    return -1;
}

static void web_drop_client(int idx)
{
    if (idx < 0 || idx >= WEB_MAX_CLIENTS) return;
    if (web_clients[idx].active) {
        web_close_socket(web_clients[idx].sock);
    }
    web_reset_client(&web_clients[idx]);
}

static int web_key_enqueue(int keycode)
{
    if (web_key_count >= WEB_KEY_QUEUE_CAP) return -1;
    web_key_queue[web_key_tail] = keycode;
    web_key_tail = (web_key_tail + 1) % WEB_KEY_QUEUE_CAP;
    web_key_count++;
    return 0;
}

static int web_key_dequeue(int *keycode)
{
    if (web_key_count <= 0) return -1;
    *keycode = web_key_queue[web_key_head];
    web_key_head = (web_key_head + 1) % WEB_KEY_QUEUE_CAP;
    web_key_count--;
    return 0;
}

static int web_keycode_is_valid(int keycode)
{
    switch (keycode) {
        case DSKY_KEY_PRO:
        case DSKY_KEY_0:
        case DSKY_KEY_1:
        case DSKY_KEY_2:
        case DSKY_KEY_3:
        case DSKY_KEY_4:
        case DSKY_KEY_5:
        case DSKY_KEY_6:
        case DSKY_KEY_7:
        case DSKY_KEY_8:
        case DSKY_KEY_9:
        case DSKY_KEY_VERB:
        case DSKY_KEY_NOUN:
        case DSKY_KEY_PLUS:
        case DSKY_KEY_MINUS:
        case DSKY_KEY_ENTR:
        case DSKY_KEY_CLR:
        case DSKY_KEY_KREL:
        case DSKY_KEY_RSET:
            return 1;
        default:
            return 0;
    }
}

static int web_parse_nonneg_int(const char *text, int *out)
{
    long val;
    const char *p;

    p = text;
    while (web_is_space(*p)) p++;
    if (!web_is_digit(*p)) return -1;

    val = 0;
    while (web_is_digit(*p)) {
        val = val * 10 + (long)(*p - '0');
        if (val > 1000000L) return -1;
        p++;
    }

    while (web_is_space(*p)) p++;
    if (*p != '\0') return -1;

    *out = (int)val;
    return 0;
}

static int web_parse_keycode_json(const char *body, int body_len, int *keycode)
{
    char tmp[WEB_MAX_BODY + 1];
    char *p;
    char *end;
    long val;

    if (body_len < 0 || body_len > WEB_MAX_BODY) return -1;
    memcpy(tmp, body, (size_t)body_len);
    tmp[body_len] = '\0';

    p = strstr(tmp, "\"keycode\"");
    if (p == NULL) return -1;
    p = strchr(p, ':');
    if (p == NULL) return -1;
    p++;
    while (web_is_space(*p)) p++;
    if (*p == '\0') return -1;

    val = strtol(p, &end, 10);
    if (p == end) return -1;
    while (web_is_space(*end)) end++;
    if (*end != '\0' && *end != '}' && *end != ',') return -1;

    if (val < -32768L || val > 32767L) return -1;
    *keycode = (int)val;
    return 0;
}

static int web_find_header_end(const char *buf, int len)
{
    int i;
    for (i = 0; i + 3 < len; i++) {
        if (buf[i] == '\r' &&
            buf[i + 1] == '\n' &&
            buf[i + 2] == '\r' &&
            buf[i + 3] == '\n') {
            return i + 4;
        }
    }
    return -1;
}

static int web_find_crlf(const char *buf, int start, int len)
{
    int i;
    for (i = start; i + 1 < len; i++) {
        if (buf[i] == '\r' && buf[i + 1] == '\n') return i;
    }
    return -1;
}

static int web_queue_bytes(web_client_t *c, const char *data, int len)
{
    int pending;

    if (len <= 0) return 0;

    if (c->tx_len == c->tx_off) {
        c->tx_len = 0;
        c->tx_off = 0;
    }

    pending = c->tx_len - c->tx_off;
    if (c->tx_off > 0 && pending + len > WEB_TX_BUF) {
        memmove(c->tx_buf, c->tx_buf + c->tx_off, (size_t)pending);
        c->tx_len = pending;
        c->tx_off = 0;
    }

    if (c->tx_len + len > WEB_TX_BUF) return -1;
    memcpy(c->tx_buf + c->tx_len, data, (size_t)len);
    c->tx_len += len;
    return 0;
}

static int web_client_pending_bytes(const web_client_t *c)
{
    return (c->tx_len - c->tx_off) + c->sse_next_len;
}

static int web_promote_next_sse_frame(web_client_t *c)
{
    if (c->sse_next_len <= 0) return 0;
    if (c->sse_next_len > WEB_TX_BUF) return -1;
    if (c->tx_len != c->tx_off) return 0;

    memcpy(c->tx_buf, c->sse_next_buf, (size_t)c->sse_next_len);
    c->tx_len = c->sse_next_len;
    c->tx_off = 0;
    c->sse_next_len = 0;
    return 0;
}

static int web_queue_response(web_client_t *c, int status,
                              const char *content_type, const char *body)
{
    char header[256];
    int body_len, header_len;

    body_len = (int)strlen(body);
    header_len = sprintf(
        header,
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %d\r\n"
        "Connection: close\r\n"
        "Cache-Control: no-cache\r\n"
        "\r\n",
        status, web_status_text(status), content_type, body_len);
    if (header_len < 0 || header_len >= (int)sizeof(header)) return -1;
    if (web_queue_bytes(c, header, header_len) != 0) return -1;
    if (web_queue_bytes(c, body, body_len) != 0) return -1;
    c->close_after_tx = 1;
    return 0;
}

static int web_queue_json_error(web_client_t *c, int status, const char *code)
{
    char body[96];
    int n;
    n = sprintf(body, "{\"error\":\"%s\"}", code);
    if (n < 0) {
        strcpy(body, "{\"error\":\"server\"}");
    } else if (n >= (int)sizeof(body)) {
        return -1;
    }
    return web_queue_response(c, status, "application/json", body);
}

static int web_build_state_json(char *out, int cap)
{
    char tmp[768];
    int n;
    n = sprintf(
        tmp,
        "{\"lights\":{\"uplink_acty\":%d,\"temp\":%d,\"key_rel\":%d,"
        "\"vel\":%d,\"no_att\":%d,\"alt\":%d,\"gimbal_lock\":%d,"
        "\"tracker\":%d,\"prog_alarm\":%d,\"stby\":%d,\"restart\":%d,"
        "\"opr_err\":%d,\"comp_acty\":%d},"
        "\"prog\":[%d,%d],\"verb\":[%d,%d],\"noun\":[%d,%d],"
        "\"r1\":{\"sign\":%d,\"digits\":[%d,%d,%d,%d,%d]},"
        "\"r2\":{\"sign\":%d,\"digits\":[%d,%d,%d,%d,%d]},"
        "\"r3\":{\"sign\":%d,\"digits\":[%d,%d,%d,%d,%d]}}",
        dsky_display.light_uplink_acty,
        dsky_display.light_temp,
        dsky_display.light_key_rel,
        dsky_display.light_vel,
        dsky_display.light_no_att,
        dsky_display.light_alt,
        dsky_display.light_gimbal_lock,
        dsky_display.light_tracker,
        dsky_display.light_prog_alarm,
        dsky_display.light_stby,
        dsky_display.light_restart,
        dsky_display.light_opr_err,
        dsky_display.light_comp_acty,
        dsky_display.prog[0], dsky_display.prog[1],
        dsky_display.verb[0], dsky_display.verb[1],
        dsky_display.noun[0], dsky_display.noun[1],
        dsky_display.r1_sign,
        dsky_display.r1[0], dsky_display.r1[1], dsky_display.r1[2],
        dsky_display.r1[3], dsky_display.r1[4],
        dsky_display.r2_sign,
        dsky_display.r2[0], dsky_display.r2[1], dsky_display.r2[2],
        dsky_display.r2[3], dsky_display.r2[4],
        dsky_display.r3_sign,
        dsky_display.r3[0], dsky_display.r3[1], dsky_display.r3[2],
        dsky_display.r3[3], dsky_display.r3[4]);
    if (n < 0 || n >= (int)sizeof(tmp) || n >= cap) return -1;
    memcpy(out, tmp, (size_t)n + 1);
    return n;
}

static int web_build_sse_snapshot_frame(char *out, int cap)
{
    char json[768];
    int json_len;
    int total_len;
    json_len = web_build_state_json(json, sizeof(json));
    if (json_len < 0) return -1;
    total_len = 6 + json_len + 2;
    if (total_len >= cap) return -1;
    memcpy(out, "data: ", 6);
    memcpy(out + 6, json, (size_t)json_len);
    out[6 + json_len] = '\n';
    out[6 + json_len + 1] = '\n';
    out[6 + json_len + 2] = '\0';
    return total_len;
}

static int web_queue_sse_frame(web_client_t *c, const char *frame, int frame_len)
{
    int pending;

    if (!c->is_sse) return 0;
    if (frame_len <= 0 || frame_len > WEB_SSE_FRAME_BUF) return -1;

    pending = c->tx_len - c->tx_off;
    if (pending == 0) {
        if (web_queue_bytes(c, frame, frame_len) != 0) return -1;
    } else {
        memcpy(c->sse_next_buf, frame, (size_t)frame_len);
        c->sse_next_len = frame_len;
    }
    return 0;
}

static int web_try_parse_request(web_client_t *c, web_request_t *req)
{
    int header_end, line_end, pos, next_line, line_len;
    int content_length;
    int total_len;
    char line[WEB_HEADER_LINE_BUF];
    char method[8];
    char path[WEB_PATH_BUF];
    char version[16];

    header_end = web_find_header_end(c->rx_buf, c->rx_len);
    if (header_end < 0) {
        if (c->rx_len >= WEB_RX_BUF) return 413;
        return 0;
    }

    line_end = web_find_crlf(c->rx_buf, 0, header_end);
    if (line_end <= 0 || line_end >= WEB_REQ_LINE_BUF) return 400;

    memcpy(line, c->rx_buf, (size_t)line_end);
    line[line_end] = '\0';

    if (sscanf(line, "%7s %127s %15s", method, path, version) < 2) {
        return 400;
    }

    req->method = WEB_METHOD_OTHER;
    if (strcmp(method, "GET") == 0) req->method = WEB_METHOD_GET;
    else if (strcmp(method, "POST") == 0) req->method = WEB_METHOD_POST;
    strncpy(req->path, path, WEB_PATH_BUF - 1);
    req->path[WEB_PATH_BUF - 1] = '\0';

    content_length = 0;
    pos = line_end + 2;
    while (pos < header_end - 2) {
        next_line = web_find_crlf(c->rx_buf, pos, header_end);
        if (next_line < 0) return 400;
        line_len = next_line - pos;
        if (line_len == 0) break;
        if (line_len >= WEB_HEADER_LINE_BUF) return 413;

        memcpy(line, c->rx_buf + pos, (size_t)line_len);
        line[line_len] = '\0';

        if (web_starts_with_ci(line, "Content-Length:")) {
            if (web_parse_nonneg_int(line + 15, &content_length) != 0) {
                return 400;
            }
        }

        pos = next_line + 2;
    }

    if (content_length > WEB_MAX_BODY) return 413;
    total_len = header_end + content_length;
    if (total_len > WEB_RX_BUF) return 413;
    if (c->rx_len < total_len) return 0;

    req->content_length = content_length;
    if (content_length > 0) {
        memcpy(req->body, c->rx_buf + header_end, (size_t)content_length);
    }
    req->body[content_length] = '\0';
    return 1;
}

static int web_handle_request(web_client_t *c, const web_request_t *req)
{
    int keycode;
    int frame_len;
    char frame[WEB_SSE_FRAME_BUF];
    static const char sse_headers[] =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/event-stream\r\n"
        "Cache-Control: no-cache\r\n"
        "Connection: keep-alive\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "\r\n";
    static const char sse_retry[] = "retry: 1000\n\n";

    if (strcmp(req->path, "/") == 0) {
        if (req->method != WEB_METHOD_GET) {
            return web_queue_json_error(c, 405, "method_not_allowed");
        }
        return web_queue_response(c, 200, "text/html; charset=utf-8", web_index_html);
    }

    if (strcmp(req->path, "/events") == 0) {
        if (req->method != WEB_METHOD_GET) {
            return web_queue_json_error(c, 405, "method_not_allowed");
        }
        if (web_queue_bytes(c, sse_headers, (int)strlen(sse_headers)) != 0) return -1;
        if (web_queue_bytes(c, sse_retry, (int)strlen(sse_retry)) != 0) return -1;
        c->is_sse = 1;
        c->close_after_tx = 0;
        c->stalled_ticks = 0;

        frame_len = web_build_sse_snapshot_frame(frame, sizeof(frame));
        if (frame_len < 0) return -1;
        if (web_queue_sse_frame(c, frame, frame_len) != 0) return -1;
        return 0;
    }

    if (strcmp(req->path, "/key") == 0) {
        if (req->method != WEB_METHOD_POST) {
            return web_queue_json_error(c, 405, "method_not_allowed");
        }
        if (web_parse_keycode_json(req->body, req->content_length, &keycode) != 0) {
            return web_queue_json_error(c, 400, "invalid_payload");
        }
        if (!web_keycode_is_valid(keycode)) {
            return web_queue_json_error(c, 400, "invalid_keycode");
        }
        if (web_key_enqueue(keycode) != 0) {
            return web_queue_json_error(c, 503, "busy");
        }
        return web_queue_response(c, 200, "application/json", "{\"ok\":true}");
    }

    return web_queue_json_error(c, 404, "not_found");
}

static int web_process_client_request(web_client_t *c)
{
    web_request_t req;
    int parse_result;

    if (c->is_sse) return 0;
    if (c->rx_len <= 0) return 0;

    parse_result = web_try_parse_request(c, &req);
    if (parse_result == 0) return 0;
    if (parse_result == 413) {
        c->rx_len = 0;
        return web_queue_json_error(c, 413, "too_large");
    }
    if (parse_result != 1) {
        c->rx_len = 0;
        return web_queue_json_error(c, 400, "bad_request");
    }

    c->rx_len = 0;
    return web_handle_request(c, &req);
}

static int web_read_client(web_client_t *c)
{
    int space;
    int n;
    int err;

    if (!c->active || c->is_sse) return 0;

    space = WEB_RX_BUF - c->rx_len;
    if (space <= 0) return -1;

    n = recv(c->sock, c->rx_buf + c->rx_len, space, 0);
    if (n > 0) {
        c->rx_len += n;
        return 0;
    }
    if (n == 0) return -1;

    err = WSAGetLastError();
    if (err == WSAEWOULDBLOCK) return 0;
    return -1;
}

static int web_flush_client(web_client_t *c)
{
    int pending;
    int n;
    int err;

    if (!c->active) return 0;

    if (c->tx_len == c->tx_off) {
        c->tx_len = 0;
        c->tx_off = 0;
        if (web_promote_next_sse_frame(c) != 0) return -1;
    }

    pending = c->tx_len - c->tx_off;
    if (pending <= 0) {
        c->stalled_ticks = 0;
        if (c->close_after_tx) return -1;
        return 0;
    }

    n = send(c->sock, c->tx_buf + c->tx_off, pending, 0);
    if (n > 0) {
        c->tx_off += n;
        c->stalled_ticks = 0;
        if (c->tx_off == c->tx_len) {
            c->tx_off = 0;
            c->tx_len = 0;
            if (web_promote_next_sse_frame(c) != 0) return -1;
        }
    } else if (n == 0) {
        return -1;
    } else {
        err = WSAGetLastError();
        if (err != WSAEWOULDBLOCK) return -1;
        c->stalled_ticks++;
    }

    if (web_client_pending_bytes(c) > 0 && c->stalled_ticks > WEB_STALL_TICKS_LIMIT) {
        return -1;
    }
    if (c->close_after_tx && web_client_pending_bytes(c) == 0) {
        return -1;
    }
    return 0;
}

static void web_reject_extra_client(SOCKET sock)
{
    static const char response[] =
        "HTTP/1.1 503 Service Unavailable\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: 16\r\n"
        "Connection: close\r\n"
        "\r\n"
        "{\"error\":\"busy\"}";
    send(sock, response, (int)strlen(response), 0);
    web_close_socket(sock);
}

static void web_accept_connections(void)
{
    int accepted;
    struct sockaddr_in addr;
    int addr_len;
    SOCKET sock;
    int err;
    int slot;

    accepted = 0;
    while (accepted < WEB_MAX_ACCEPTS_PER_TICK) {
        addr_len = (int)sizeof(addr);
        sock = accept(web_listen_sock, (struct sockaddr *)&addr, &addr_len);
        if (sock == INVALID_SOCKET) {
            err = WSAGetLastError();
            if (err == WSAEWOULDBLOCK) break;
            break;
        }

        accepted++;
        if (web_set_nonblocking(sock) != 0) {
            web_close_socket(sock);
            continue;
        }

        slot = web_find_free_client_slot();
        if (slot < 0) {
            web_reject_extra_client(sock);
            continue;
        }

        web_reset_client(&web_clients[slot]);
        web_clients[slot].sock = sock;
        web_clients[slot].active = 1;
    }
}

static void web_broadcast_snapshot(void)
{
    char frame[WEB_SSE_FRAME_BUF];
    int frame_len;
    int i;

    frame_len = web_build_sse_snapshot_frame(frame, sizeof(frame));
    if (frame_len < 0) return;

    for (i = 0; i < WEB_MAX_CLIENTS; i++) {
        if (web_clients[i].active && web_clients[i].is_sse) {
            if (web_queue_sse_frame(&web_clients[i], frame, frame_len) != 0) {
                web_drop_client(i);
            }
        }
    }
}

static void web_maybe_send_heartbeat(void)
{
    int i;
    static const char heartbeat[] = ": keepalive\n\n";

    web_heartbeat_counter++;
    if (web_heartbeat_counter < WEB_HEARTBEAT_TICKS) return;
    web_heartbeat_counter = 0;

    for (i = 0; i < WEB_MAX_CLIENTS; i++) {
        if (web_clients[i].active && web_clients[i].is_sse) {
            if (web_client_pending_bytes(&web_clients[i]) == 0) {
                if (web_queue_bytes(&web_clients[i], heartbeat,
                                    (int)strlen(heartbeat)) != 0) {
                    web_drop_client(i);
                }
            }
        }
    }
}

static void web_init_server(void)
{
    WSADATA wsa_data;
    struct sockaddr_in addr;
    int i;
    int on;
    int html_len;

    /* Keep a hard bound: root page + response headers must fit TX buffer. */
    html_len = (int)strlen(web_index_html);
    if (html_len + 256 > WEB_TX_BUF) {
        fprintf(stderr, "Web backend init failed: WEB_TX_BUF too small for index HTML\n");
        exit(1);
    }

    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
        fprintf(stderr, "Web backend init failed: WSAStartup error\n");
        exit(1);
    }

    web_listen_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (web_listen_sock == INVALID_SOCKET) {
        fprintf(stderr, "Web backend init failed: socket error %d\n", WSAGetLastError());
        WSACleanup();
        exit(1);
    }

    on = 1;
    setsockopt(web_listen_sock, SOL_SOCKET, SO_REUSEADDR, (const char *)&on, sizeof(on));

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(WEB_PORT);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    if (bind(web_listen_sock, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
        fprintf(stderr, "Web backend init failed: bind error %d\n", WSAGetLastError());
        web_close_socket(web_listen_sock);
        web_listen_sock = INVALID_SOCKET;
        WSACleanup();
        exit(1);
    }

    if (listen(web_listen_sock, WEB_MAX_CLIENTS) != 0) {
        fprintf(stderr, "Web backend init failed: listen error %d\n", WSAGetLastError());
        web_close_socket(web_listen_sock);
        web_listen_sock = INVALID_SOCKET;
        WSACleanup();
        exit(1);
    }

    if (web_set_nonblocking(web_listen_sock) != 0) {
        fprintf(stderr, "Web backend init failed: nonblocking error %d\n", WSAGetLastError());
        web_close_socket(web_listen_sock);
        web_listen_sock = INVALID_SOCKET;
        WSACleanup();
        exit(1);
    }

    for (i = 0; i < WEB_MAX_CLIENTS; i++) {
        web_reset_client(&web_clients[i]);
    }

    web_key_head = 0;
    web_key_tail = 0;
    web_key_count = 0;
    web_heartbeat_counter = 0;
    web_prev_display_valid = 0;
    web_running = 1;

    printf("Web backend listening on http://127.0.0.1:%d/\n", WEB_PORT);
}

static void web_update_server(void)
{
    int i;

    if (!web_running) return;

    web_accept_connections();

    for (i = 0; i < WEB_MAX_CLIENTS; i++) {
        if (!web_clients[i].active) continue;
        if (web_read_client(&web_clients[i]) != 0) {
            web_drop_client(i);
            continue;
        }
        if (!web_clients[i].active) continue;
        if (web_process_client_request(&web_clients[i]) != 0) {
            web_drop_client(i);
        }
    }

    if (!web_prev_display_valid ||
        memcmp(&dsky_display, &web_prev_display, sizeof(dsky_display)) != 0) {
        web_prev_display = dsky_display;
        web_prev_display_valid = 1;
        web_broadcast_snapshot();
    }

    web_maybe_send_heartbeat();

    for (i = 0; i < WEB_MAX_CLIENTS; i++) {
        if (!web_clients[i].active) continue;
        if (web_flush_client(&web_clients[i]) != 0) {
            web_drop_client(i);
        }
    }
}

static void web_poll_key_input(void)
{
    int keycode;
    while (web_key_dequeue(&keycode) == 0) {
        dsky_submit_key(keycode);
    }
}

static void web_cleanup_server(void)
{
    int i;
    if (!web_running) return;

    for (i = 0; i < WEB_MAX_CLIENTS; i++) {
        if (web_clients[i].active) {
            web_drop_client(i);
        }
    }

    web_close_socket(web_listen_sock);
    web_listen_sock = INVALID_SOCKET;
    WSACleanup();
    web_running = 0;
}

static void web_sleep(int ms)
{
    Sleep(ms);
}

dsky_backend_t dsky_web_backend = {
    web_init_server,
    web_update_server,
    web_poll_key_input,
    web_cleanup_server,
    web_sleep
};

#else

/*
 * Non-Windows stub to keep all-platform builds working.
 * The web backend is intentionally Windows-first in this phase.
 */

#include <string.h>

static void web_stub_init(void) { }
static void web_stub_update(void) { }
static void web_stub_poll(void) { }
static void web_stub_cleanup(void) { }
static void web_stub_sleep(int ms) { (void)ms; }

dsky_backend_t dsky_web_backend = {
    web_stub_init,
    web_stub_update,
    web_stub_poll,
    web_stub_cleanup,
    web_stub_sleep
};

#endif /* _WIN32 */
