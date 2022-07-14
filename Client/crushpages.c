#ifdef CC_BUILD_WIN
#define CC_API __declspec(dllimport)
#define CC_VAR __declspec(dllimport)
#else
#define CC_API
#define CC_VAR
#endif

#include "Chat.h"
#include "Game.h"
#include "String.h"
#include "Server.h"
#include "Input.h"
#include "Event.h"
#include "Protocol.h"

#define CRP_PLUGIN_VER "1"
#define CRP_WELCOME "&eUsing &8ya_&coi_ &fv"
#define CRP_HANDSHAKE "yaoi\0"

static struct _ServerConnectionData *Server_;
static struct _InputEventsList *InputEvents_;
static struct _WorldEventsList *WorldEvents_;
static struct _NetEventsList *NetEvents_;

/*########################################################################################################################*
 *------------------------------------------------------Dynamic loading----------------------------------------------------*
 *#########################################################################################################################*/
#define QUOTE(x) #x

#ifdef CC_BUILD_WIN
#define WIN32_LEAN_AND_MEAN
#define NOSERVICE
#define NOMCX
#define NOIME
#include <windows.h>
#define LoadSymbol(name) name##_ = GetProcAddress(app, QUOTE(name))
#else
#define _GNU_SOURCE
#include <dlfcn.h>
#define LoadSymbol(name) name##_ = dlsym(RTLD_DEFAULT, QUOTE(name))
#endif

static void dummy(void *obj, int c)
{
    return;
}
/* might need those later
void debug1(const char *msg)
{
    char bfr2[254];
    cc_string msg2;
    String_InitArray(msg2, bfr2);
    String_AppendConst(&msg2, msg);
    Chat_Add(&msg2);
}

void debug2(int msg)
{
    char bfr2[254];
    cc_string msg2;
    String_InitArray(msg2, bfr2);
    String_AppendInt(&msg2, msg);
    Chat_Add(&msg2);
}
*/
void Event_RaiseInput(struct Event_Input *handlers, int key, cc_bool repeating)
{
    int i;
    for (i = 0; i < handlers->Count; i++)
    {
        handlers->Handlers[i](handlers->Objs[i], key, repeating);
    }
}

typedef struct
{
    // 0 - plain, 1 - alt, 2 - shift, 3 - alt shift
    cc_uint8 ascii[4];
} bind;

struct layout
{
    char monicker[4];
    int len;
    bind binds[122];
};

struct inputState
{
    cc_bool active;
    cc_bool uploadState;
    cc_bool shiftPressed;
    cc_bool altPressed;
    cc_bool ctrlPressed;
    cc_bool chatOpen;
    cc_bool handshaking;
    cc_uint8 layoutNum;
    cc_uint8 langSwitch;
    struct layout layouts[8];
    void *pressFunc;

} static state1;

void addBind(cc_uint8 key, cc_uint8 ascii0, cc_uint8 ascii1, cc_uint8 ascii2, cc_uint8 ascii3)
{
    if (!state1.layoutNum)
    {

        return;
    }

    cc_uint8 layout = state1.layoutNum - 1;
    cc_uint8 *move = state1.layouts[layout].binds[key].ascii;
    (*move++ = ascii0, *move++ = ascii1, *move++ = ascii2, *move++ = ascii3);
    state1.layouts[layout].len++;
}

void addLayout(char monicker[3])
{
    for (int i = 0; i < 3; i++)
    {
        state1.layouts[state1.layoutNum].monicker[i] = monicker[i];
    }
    state1.layouts[state1.layoutNum].monicker[4] = '\0';
    state1.layouts[state1.layoutNum].len = 0;
    state1.layoutNum++;
}

void switchLayout()
{
    if (state1.layoutNum > 1)
    {
        state1.langSwitch++;
        state1.langSwitch %= (state1.layoutNum);
    }
}

void magic(int pos)
{
    if (state1.chatOpen == true)
    {

        InputEvents_->Press.Handlers[0] = state1.pressFunc;
        if (pos == KEY_SPACE)
        {
            Event_RaiseInt(&InputEvents_->Press, 32);
            InputEvents_->Press.Handlers[0] = dummy;
        }
        if (state1.layouts[state1.langSwitch].binds[pos].ascii[0] == 0)
        {
            InputEvents_->Press.Handlers[0] = dummy;
            return;
        }
        if (state1.altPressed)
        {
            Event_RaiseInt(&InputEvents_->Press, state1.layouts[state1.langSwitch].binds[pos].ascii[1]);
        }
        else if (state1.shiftPressed)
        {
            Event_RaiseInt(&InputEvents_->Press, state1.layouts[state1.langSwitch].binds[pos].ascii[2]);
        }
        else if (state1.shiftPressed && state1.altPressed)
        {
            Event_RaiseInt(&InputEvents_->Press, state1.layouts[state1.langSwitch].binds[pos].ascii[3]);
        }
        else
        {
            Event_RaiseInt(&InputEvents_->Press, state1.layouts[state1.langSwitch].binds[pos].ascii[0]);
        }

        InputEvents_->Press.Handlers[0] = dummy;
    }
}

static void downMgr(void *obj, int c)
{
    switch (c)
    {
    case 94:
        return;
        break;
    case 36:
    case 37:
        if (state1.ctrlPressed == true)
        {
            switchLayout();
        }
        state1.shiftPressed = true;
        return;
        break;
    case 38:
    case 39:
        if (state1.shiftPressed == true)
        {
            switchLayout();
        }
        state1.ctrlPressed = true;
        return;
        break;
    case 41:
        state1.altPressed = true;
        return;
        break;
    case 91:
    case 97:
    case 98:
    case 99:
    case 1001:
    case 1002:
        return;
        break;
    }

    if ((c >= 0 && c <= 20) || (c >= 42 && c <= 47) || (c >= 117 && c <= 122))
        return;

    magic(c);

    if ((KeyBind_IsPressed(KEYBIND_CHAT) || c == KEY_SLASH) && state1.chatOpen == false)
    {

        state1.chatOpen = true;
        InputEvents_->Press.Handlers[0] = dummy;
    }

    if (state1.chatOpen == true)
    {
        if ((KeyBind_IsPressed(KEYBIND_SEND_CHAT) || c == KEY_ESCAPE))
        {
            state1.chatOpen = false;
            InputEvents_->Press.Handlers[0] = state1.pressFunc;
        }
    }
}

static void upMgr(void *obj, int c)
{

    if (c == 38 || c == 39)
    {
        state1.ctrlPressed = false;
        return;
    }

    if (c == 36 || c == 37)
    {
        state1.shiftPressed = false;
        return;
    }

    if (c == 41)
    {
        state1.altPressed = false;
        return;
    }
}

void enlayout()
{
    addLayout("ENG");
    for (int i = KEY_A; i <= KEY_Z; i++)
    {
        addBind(i, i + 32, i + 32, i, i);
    }
    for (int i = KEY_2; i <= KEY_5; i++)
    {
        addBind(i, i, i, i - 10, i - 10);
    }
    addBind(KEY_TILDE, '`', '`', '~', '~');
    addBind(KEY_COMMA, ',', ',', '<', '<');
    addBind(KEY_PERIOD, '.', '.', '>', '>');
    addBind(KEY_SLASH, '/', '/', '?', '?');
    addBind(KEY_0, '0', '0', ')', ')');
    addBind(KEY_9, '9', '9', '(', '(');
    addBind(KEY_8, '8', '8', '*', '*');
    addBind(KEY_7, '7', '7', '&', '&');
    addBind(KEY_6, '6', '6', '^', '^');
    addBind(KEY_4, '4', '4', '$', '$');
    addBind(KEY_1, '1', '1', '!', '!');
    addBind(KEY_MINUS, '-', '-', '_', '_');
    addBind(KEY_EQUALS, '=', '=', '+', '+');
    addBind(KEY_SEMICOLON, ';', ';', ':', ':');
    addBind(KEY_QUOTE, '\'', '\'', '"', '"');
    addBind(KEY_LBRACKET, '[', '[', '{', '{');
    addBind(KEY_RBRACKET, ']', ']', '}', '}');
    addBind(KEY_BACKSLASH, '\\', '\\', '|', '|');
}

void onWorldLoad()
{
    cc_uint8 stuff[4] = {121, 97, 111, 105};
    CPE_SendPluginMessage(255, stuff);
}

void activateYaoi()
{
    LoadSymbol(Server);
    LoadSymbol(InputEvents);

    state1.pressFunc = InputEvents_->Press.Handlers[0];

    Event_Register_(&InputEvents_->Down, NULL, downMgr);
    Event_Register_(&InputEvents_->Up, NULL, upMgr);

    state1.chatOpen = false;
    state1.layoutNum = 0;
    state1.shiftPressed = false;
    state1.altPressed = false;

    enlayout();

    state1.langSwitch = 0;
}

void onPluginMessage(void *obj, cc_uint8 channel, cc_uint8 *data)
{
    char dataHandshake[5] = {data[0], data[1], data[2], data[3], data[4]};

    if (!state1.handshaking)
    {

        state1.handshaking = true;
        for (int i = 0; (i < 5) && (state1.handshaking == true); i++)
        {
            if (dataHandshake[i] != CRP_HANDSHAKE[i])
                state1.handshaking = false;
        }

        if (state1.handshaking && (!state1.active))
        {
            state1.active = true;
            activateYaoi();
        }
    }
    else
    {
        for (int i = 0; i < 64; i++)
        {
            cc_uint8 c = data[i];
            if (c == '*')
            {
                char layName[] = {data[i + 1], data[i + 2], data[i + 3]};
                addLayout(layName);
                i += 3;
            }
            else if(c == 255){
                state1.handshaking = false;
            }
            else if (c != 0)
            {
                char layChar[] = {c, data[i + 1], data[i + 2], data[i + 3], data[i + 4]};
                addBind(layChar[0], layChar[1], layChar[2], layChar[3], layChar[4]);
                i += 4;
            }
        }
    }
}

static void crushPages_Init(void)
{
    state1.handshaking = false;
    state1.active = false;
    cc_string msg;
    char bfr[sizeof(CRP_WELCOME) + sizeof(CRP_PLUGIN_VER)];
    String_InitArray(msg, bfr);
    String_AppendConst(&msg, CRP_WELCOME CRP_PLUGIN_VER);
    Chat_Add(&msg);

    LoadSymbol(WorldEvents);
    Event_Register_(&WorldEvents_->MapLoaded, NULL, onWorldLoad);

    LoadSymbol(NetEvents);
    Event_Register_(&NetEvents_->PluginMessageReceived, NULL, onPluginMessage);
}

#ifdef CC_BUILD_WIN
// special attribute to get symbols exported with Visual Studio
#define PLUGIN_EXPORT __declspec(dllexport)
#else
// public symbols already exported when compiling shared lib with GCC
#define PLUGIN_EXPORT
#endif

PLUGIN_EXPORT int Plugin_ApiVersion = 1;
PLUGIN_EXPORT struct IGameComponent Plugin_Component = {
    crushPages_Init /* Init */
};
