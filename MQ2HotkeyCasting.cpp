// MQ2HotkeyCasting.cpp : Defines the entry point for the DLL application.
//

// PLUGIN_API is only to be used for callbacks.  All existing callbacks at this time
// are shown below. Remove the ones your plugin does not use.  Always use Initialize
// and Shutdown for setup and cleanup.

#include <mq/Plugin.h>

PreSetup("MQ2HotkeyCasting");
PLUGIN_VERSION(0.1);

uintptr_t MQ2HotkeyCasting_AllowSlashCommand;
/**
 * Avoid Globals if at all possible, since they persist throughout your program.
 * But if you must have them, here is the place to put them.
 */
// bool ShowMQ2HotkeyCastingWindow = true;
#if defined(ROF2EMU)
//Brainiac MVP in supplying this, I just put it together. 
//32bit/RoF2 client signature is different than live/64bit.
class MQ2HotkeyCasting_CHotButtonWnd_Hook
{
public:
	DETOUR_TRAMPOLINE_DEF(void, DoHotButton_Trampoline, (int button, BOOL click));

	void DoHotButton_Detour(int button, BOOL click)
	{
		//ItemPending is incremented during events, like casting a spell, so its set to zero temporarly while the method is called
		//it needs to be zero or you will get "You can't use that command right now..."
		int oldItemPending = std::exchange(pEverQuestInfo->ItemPending, 0);
		//call original method with 0 itempending now
		DoHotButton_Trampoline(button, click);
		//replace the item pending back to the way it was. 
		pEverQuestInfo->ItemPending = oldItemPending;
	}
};

class MQ2HotkeyCasting_AllowSlashCommand_Hook
{
public:
	DETOUR_TRAMPOLINE_DEF(int, AllowSlashCommand_Trampoline, (void));

	int AllowSlashCommand_Detour(void)
	{
		//WriteChatf("\agForcing AllowSlashCommand");

		return 1;
	}
};
#else
//64bit Live client signature
class CHotButtonWnd_Hook
{
public:
	DETOUR_TRAMPOLINE_DEF(void, DoHotButton_Trampoline, (int button, BOOL click, const KeyCombo& combo));

	void DoHotButton_Detour(int button, BOOL click, const KeyCombo& combo)
	{
		int oldItemPending = std::exchange(pEverQuestInfo->ItemPending, 0);

		DoHotButton_Trampoline(button, click, combo);

		pEverQuestInfo->ItemPending = oldItemPending;
	}
};
#endif
/**
 * @fn InitializePlugin
 *
 * This is called once on plugin initialization and can be considered the startup
 * routine for the plugin.
 */
PLUGIN_API void InitializePlugin()
{

	DebugSpewAlways("MQ2HotkeyCasting::Initializing version %f", MQ2Version);
	WriteChatf("\agTrying to Detour DoHotButton");
	EzDetour(CHotButtonWnd__DoHotButton, &MQ2HotkeyCasting_CHotButtonWnd_Hook::DoHotButton_Detour, &MQ2HotkeyCasting_CHotButtonWnd_Hook::DoHotButton_Trampoline);
	#if defined(ROF2EMU)
		//allow things like /invite <whoever> while casting
		MQ2HotkeyCasting_AllowSlashCommand = FixEQGameOffset(0x4DE9E0);
		EzDetour(MQ2HotkeyCasting_AllowSlashCommand, &MQ2HotkeyCasting_AllowSlashCommand_Hook::AllowSlashCommand_Detour, &MQ2HotkeyCasting_AllowSlashCommand_Hook::AllowSlashCommand_Trampoline);
	#endif
	// Examples:
	// AddCommand("/mycommand", MyCommand);
	// AddXMLFile("MQUI_MyXMLFile.xml");
	// AddMQ2Data("mytlo", MyTLOData);
}

/**
 * @fn ShutdownPlugin
 *
 * This is called once when the plugin has been asked to shutdown.  The plugin has
 * not actually shut down until this completes.
 */
PLUGIN_API void ShutdownPlugin()
{
	DebugSpewAlways("MQ2HotkeyCasting::Shutting down");
	RemoveDetour(CHotButtonWnd__DoHotButton);
	#if defined(ROF2EMU)
		RemoveDetour(MQ2HotkeyCasting_AllowSlashCommand);
	#endif
	// Examples:
	// RemoveCommand("/mycommand");
	// RemoveXMLFile("MQUI_MyXMLFile.xml");
	// RemoveMQ2Data("mytlo");
}