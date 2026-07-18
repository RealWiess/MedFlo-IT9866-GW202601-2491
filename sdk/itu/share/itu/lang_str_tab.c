#include "ite/lang_str_tab.h"

const wchar_t* ITULanguage_String_Table[ITU_LANGUAGE_STRING_TSIZE][ITU_LANGUAGE_STRING_TSIZE] = {
	{L"EN", L"Simp", L"GreatBoss"}, //--> [lang = 0]
	{L"CT", L"繁體", L"龍龜聖獸"},  //--> [lang = 1]
	{L"CS", L"简体", L"龙龟圣兽"},  //--> [lang = 2]...etc, now you can add more index/item as you need
};

//for this sample, when you edit with DrawRocker
//if you set one Text widget with (Text = 2), (Text1 = 1), (Text2 = 1)
//and set UseLangTableFile = true
//
// for language 0 (default)
// and because you set Text = 2
// Text will show (lang = 0)/(index = 2) --> "GreatBoss"
//
// for language 1 --> ituSceneUpdate(xxx, ITU_EVENT_LANGUAGE, 1, 0, 0);
// and because you set Text1 = 1
// Text will show (lang = 1)/(index = 1) --> "簡體"
//
// for language 2 --> ituSceneUpdate(xxx, ITU_EVENT_LANGUAGE, 2, 0, 0);
// and because you set Text2 = 1
// Text will show (lang = 2)/(index = 1) --> "简体"
