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
#include "Platform.h"
#include "Screens.h"
#include "Widgets.h"
#include "Drawer2D.h"
#include "Graphics.h"
#include "Funcs.h"

#define PLUGIN_VER "2"
#define WELCOME_MESSAGE "&eUsing &8fake&Unicode &fv"
#define HANDSHAKE_MESSAGE "gimme the unicode bindings"

static struct _ServerConnectionData *Server_;
static struct _InputEventsList *InputEvents_;
static struct _NetEventsList *NetEvents_;

void fakeUnicodeSupport_Init();
void fakeUnicodeSupport_Free();
void fakeUnicodeSupport_onMapLoad();

// reimplementing the chat screen to mess with it later
#define CHAT_MAX_STATUS Array_Elems(Chat_Status)
#define CHAT_MAX_BOTTOMRIGHT Array_Elems(Chat_BottomRight)
#define CHAT_MAX_CLIENTSTATUS Array_Elems(Chat_ClientStatus)

struct ChatScreen
{
    Screen_Body float chatAcc;
    cc_bool suppressNextPress;
    int chatIndex, paddingX, paddingY;
    int lastDownloadStatus;
    struct FontDesc chatFont, announcementFont, bigAnnouncementFont, smallAnnouncementFont;
    struct TextWidget announcement, bigAnnouncement, smallAnnouncement;
    struct ChatInputWidget input;
    struct TextGroupWidget status, bottomRight, chat, clientStatus;
    struct SpecialInputWidget altText;
#ifdef CC_BUILD_TOUCH
    struct ButtonWidget send, cancel, more;
#endif

    struct Texture statusTextures[CHAT_MAX_STATUS];
    struct Texture bottomRightTextures[CHAT_MAX_BOTTOMRIGHT];
    struct Texture clientStatusTextures[CHAT_MAX_CLIENTSTATUS];
    struct Texture chatTextures[GUI_MAX_CHATLINES];
};

// dynamic symbol loading
#define QUOTE(x) #x

#ifdef CC_BUILD_WIN
#define WIN32_LEAN_AND_MEAN
#define NOSERVICE
#define NOMCX
#define NOIME
#include <windows.h>
HMODULE app;
#define LoadSymbol(name) name##_ = GetProcAddress(app, QUOTE(name))
#else
#define _GNU_SOURCE
#include <dlfcn.h>
#define LoadSymbol(name) name##_ = dlsym(RTLD_DEFAULT, QUOTE(name))
#endif

// dummy function for handler manipulation
static void dummy()
{
    return;
}

// dirty debug function
void debug1(const char *msg)
{
    char bfr2[254];
    cc_string msg2;
    String_InitArray(msg2, bfr2);
    String_AppendConst(&msg2, msg);
    Chat_Add(&msg2);
}


// raising inputs like this is not yet exposed to CC_API
void Event_RaiseInput(struct Event_Input *handlers, int key, cc_bool repeating)
{
    int i;
    for (i = 0; i < handlers->Count; i++)
    {
        handlers->Handlers[i](handlers->Objs[i], key, repeating);
    }
}

// binds any code point to a cp437 character
// the entire character bind system is BST, hence the pointers
typedef struct charBind
{
    int cp;
    cc_uint8 cp437;
    struct charBind *left;
    struct charBind *right;
} charBind;

struct buffer
{
    char buffer[8];
    int fit;
};

// keeps the state of the game
struct pluginState
{
    struct buffer buffer;
    cc_bool replaceToggle;
    cc_bool chatOpen;
    cc_bool handshaking;
    struct Screen *chatScreen;
    charBind *root;
    void *pressFunc;

} static plugin;

// search the node for the given codepoint
charBind *search(charBind *node, int cp)
{
    
    if (node == NULL || node->cp == cp)
        return node;

    if (node->cp < cp)
        return search(node->right, cp);

    return search(node->left, cp);
}

// wrapper around search() for the plugin BST root
// returns the bound CP437 symbol
cc_uint8 lookFor(int cp)
{
    charBind *result = search(plugin.root, cp);
    if (result == NULL)
        return 0;
    return result->cp437;
}

// create a new bind object
// allocates memory on ClassiCube heap memory
charBind *newBind(int cp, cc_uint8 cp437)
{
    charBind *temp = (charBind *)Mem_TryAlloc(1, sizeof(charBind));
    if (temp == NULL)
    {
        debug1("memory allocation error. fakeUnicodeSupport crashed.");
        fakeUnicodeSupport_Free();
    }
    temp->cp = cp;
    temp->cp437 = cp437;
    temp->left = temp->right = NULL;
    return temp;
}

// add a new bind from Unicode to CP437 starting from node
charBind *insertBind(charBind *node, int cp, cc_uint8 cp437)
{

    if (node == NULL)
        return newBind(cp, cp437);

    if (cp < node->cp)
        node->left = insertBind(node->left, cp, cp437);
    else if (cp > node->cp)
        node->right = insertBind(node->right, cp, cp437);

    return node;
}

// wrapper around insertBind for the plugin BST root
void addBind(int cp, cc_uint8 cp437)
{
    
    plugin.root = insertBind(plugin.root, cp, cp437);
}

// frees memory from a binary tree
void deleteTree(charBind *node)
{
    if (node == NULL)
        return;

    deleteTree(node->left);
    deleteTree(node->right);

    Mem_Free(node);
}

void KeyPressManager(void *obj, int cp)
{
    cc_uint8 c = 0;
    if (!Convert_TryCodepointToCP437(cp, &c))
    {
        c = lookFor(cp);
    }
    if (!c)
        return;
    plugin.chatScreen->VTABLE->HandlesKeyPress(plugin.chatScreen, c);
}

void KeyDownManager(void *obj, int c)
{
    if ((KeyBind_IsPressed(KEYBIND_CHAT) || c == KEY_SLASH) && plugin.chatOpen == false)
    {
        plugin.chatScreen = Gui_GetInputGrab();

        plugin.chatOpen = true;
        InputEvents_->Press.Handlers[0] = dummy;
        Event_Register_(&InputEvents_->Press, NULL, KeyPressManager);
    }

    if (plugin.chatOpen)
    {
        if ((KeyBind_IsPressed(KEYBIND_SEND_CHAT) || c == KEY_ESCAPE))
        {
            Event_Unregister_(&InputEvents_->Press, NULL, KeyPressManager);
            plugin.chatOpen = false;
            InputEvents_->Press.Handlers[0] = plugin.pressFunc;
        }
    }
}

void PluginMessageManager(void *obj, cc_uint8 channel, cc_uint8 *data)
{
    if (!plugin.handshaking)
    {
        plugin.handshaking = true;
        for (int i = 0; (i < sizeof(HANDSHAKE_MESSAGE)) && (plugin.handshaking == true); i++)
        {
            if (data[i] != HANDSHAKE_MESSAGE[i])
            {
                plugin.handshaking = false;
                Event_Unregister_(&NetEvents_->PluginMessageReceived, NULL, PluginMessageManager);
                break;
            }
        }
        if (plugin.handshaking)
        {
            plugin.buffer.fit = 0;
        }
    }
    else
    {
        for (int i = 0; i < 64; i++)
        {
            plugin.buffer.buffer[plugin.buffer.fit] = data[i];
            plugin.buffer.fit++;
            if (plugin.buffer.fit >= 8)
            {
                int *cast = (int *)plugin.buffer.buffer;
                int *cast2 = cast + 1;

                if (((*cast) != -1) && (((*cast2) != -1)))
                {
                    addBind(*cast, *cast2);
                    plugin.buffer.fit = 0;
                }
                else
                {
                    plugin.handshaking = false;
                    break;
                }
            }
        }
    }
}

void fakeUnicodeSupport_Free()
{
    deleteTree(plugin.root);
}

void handshake()
{
    cc_uint8 theMessage[64];

    for (int i = 0; i < 64; i++)
    {
        if (i < sizeof(HANDSHAKE_MESSAGE))
        {
            theMessage[i] = HANDSHAKE_MESSAGE[i];
        }
        else
        {
            theMessage[i] = 0;
        }
    }

    CPE_SendPluginMessage(255, theMessage);
}

void fakeUnicodeSupport_onMapLoad()
{
    plugin.pressFunc = InputEvents_->Press.Handlers[0];
    Event_Register_(&InputEvents_->Down, NULL, KeyDownManager);

    handshake();

    addBind(1081, 149);
    addBind(1094, 163);
}

void fakeUnicodeSupport_Init()
{
    LoadSymbol(Server);
    LoadSymbol(InputEvents);
    LoadSymbol(NetEvents);

    plugin.chatOpen = false;
    plugin.handshaking = false;
    plugin.replaceToggle = false;
    plugin.root = NULL;
    Event_Register_(&NetEvents_->PluginMessageReceived, NULL, PluginMessageManager);
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
    fakeUnicodeSupport_Init,      /* Init */
    fakeUnicodeSupport_Free,      // free
    dummy,                        // on server reconnect
    dummy,                        // on map starts loading
    fakeUnicodeSupport_onMapLoad, // on map finishes loading
};
