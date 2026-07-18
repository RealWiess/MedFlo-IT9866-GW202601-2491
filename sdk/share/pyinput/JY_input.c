// need to install VS90SP1-KB980263-x86.exe for vs2008
//#pragma execution_character_set("utf-8")

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

// need to install VS90SP1-KB980263-x86.exe for vs2008
#pragma execution_character_set("utf-8")

//注音符號與標準鍵盤對照表
// ㄅ:1  ㄆ:Q  ㄇ:A  ㄈ:Z  ㄉ:2  ㄊ:W  ㄋ:S  ㄌ:X
// ㄍ:E  ㄎ:D  ㄏ:C  ㄐ:R  ㄑ:F  ㄒ:V  ㄓ:5  ㄔ:T
// ㄕ:G  ㄖ:B  ㄗ:Y  ㄘ:H  ㄙ:N  ㄧ:U  ㄨ:J  ㄩ:M
// ㄚ 8  ㄛ:I  ㄜ:K  ㄝ:o  ㄞ:9  ㄟ:O  ㄠ:L  ㄡ:.
// ㄢ 0  ㄣ:P  ㄤ:;  ㄥ:/  ㄦ:-
// 二聲: 6  三聲: 3  四聲: 4  入聲: 7

//"注音输入法汉字排列表,码表(mb)"

//--------------------------------------------------------------------------------
//====ba (ㄅㄚ)
static const uint8_t JY_mb_ba[]= {"八仈叭吧巴扒捌疤笆粑芭豝"};
static const uint8_t JY_mb_ba2[]= {"拔茇菝跋魃"};
static const uint8_t JY_mb_ba3[]= {"把鈀钯靶"};
static const uint8_t JY_mb_ba4[]= {"壩坝把灞爸猈罷罢耙霸"};
static const uint8_t JY_mb_ba5[]= {"吧罷罢"};
//====bo (ㄅㄛ)
static const uint8_t JY_mb_bo[]= {"剝剥嶓撥拨播波玻砵缽钵菠餑饽"};
static const uint8_t JY_mb_bo2[]= {"伯僰勃博帛搏桲泊浡渤礡礴箔脖膊舶葧薄襮踣鈸钹鉑铂鋍駁驳駮驳鵓鹁"};
static const uint8_t JY_mb_bo3[]= {"跛簸"};
static const uint8_t JY_mb_bo4[]= {"擘柏檗簸薄譒"};
static const uint8_t JY_mb_bo5[]= {"蔔卜"};
//====bai (ㄅㄞ)
static const uint8_t JY_mb_bai[]= {"掰"};
static const uint8_t JY_mb_bai2[]= {"白"};
static const uint8_t JY_mb_bai3[]= {"百佰捭擺摆柏粨襬"};
static const uint8_t JY_mb_bai4[]= {"拜敗败唄呗捭稗粺"};
//====bei (ㄅㄟ)
static const uint8_t JY_mb_bei[]= {"卑悲背波杯盃碑揹痺埤陂庳伓椑"};
static const uint8_t JY_mb_bei3[]= {"北"};
static const uint8_t JY_mb_bei4[]= {"貝被備排背輩倍北菩拔臂葡憊蓓跋鋇梖狽悖焙琲棓蜚孛"};
//====bao (ㄅㄠ)
static const uint8_t JY_mb_bao[]= {"包胞炮剝褒鮑葆苞孢煲"};
static const uint8_t JY_mb_bao2[]= {"薄雹 "};
static const uint8_t JY_mb_bao3[]= {"保寶飽堡葆褓鴇"};
static const uint8_t JY_mb_bao4[]= {"報抱暴爆袍豹瀑鮑刨煲"};
//====ban (ㄅㄢ)
static const uint8_t JY_mb_ban[]= {"班般搬辨斑頒扳"};
static const uint8_t JY_mb_ban3[]= {"板版般闆阪鈑舨粄"};
static const uint8_t JY_mb_ban4[]= {"辦半伴辯辨扮絆瓣拌"};
//====ben (ㄅㄣ)
static const uint8_t JY_mb_ben[]= {"奔賁栟"};
static const uint8_t JY_mb_ben3[]= {"本苯畚"};
static const uint8_t JY_mb_ben4[]= {"笨奔夯"};
//====bang (ㄅㄤ)
static const uint8_t JY_mb_bang[]= {"幫邦梆"};
static const uint8_t JY_mb_bang3[]= {"綁榜膀螃紡牓"};
static const uint8_t JY_mb_bang4[]= {"棒旁謗榜膀傍並磅蚌鎊棓蒡"};
//====beng (ㄅㄥ)
static const uint8_t JY_mb_beng[]= {"崩繃塴嘣奟弸綳抨絣榜嵭伻閍瞓"};
static const uint8_t JY_mb_beng2[]= {"甭"};
static const uint8_t JY_mb_beng3[]= {"繃唪洴"};
static const uint8_t JY_mb_beng4[]= {"蹦繃绷蜯蚌迸洴跰泵搒"};
//====bi (ㄅㄧ)
static const uint8_t JY_mb_bi[]= {"逼紕屄皕"};
static const uint8_t JY_mb_bi2[]= {"鼻"};
static const uint8_t JY_mb_bi3[]= {"比筆罷彼肥鄙俾匕紕枇庳鞞妣柀"};
static const uint8_t JY_mb_bi4[]= {"必畢比被費服波避秘閉幣碧敝壁弊臂斃脾拂嗶蔽泌璧庇辟跛瞥痺贔馥弼躄埤愎婢紕陛賁畀裨睥枇陂痹薜篳苾檗疪蓽陴狴"};
//====bei (ㄅㄧㄝ)
static const uint8_t JY_mb_bie[]= {"鱉憋癟虌"};
static const uint8_t JY_mb_bie2[]= {"別秘蛂蹩"};
static const uint8_t JY_mb_bie3[]= {"癟蛂"};
static const uint8_t JY_mb_bie4[]= {"彆"};
//====biao (ㄅㄧㄠ)
static const uint8_t JY_mb_biao[]= {"標漂飆鏢彪鑣苞鏖滮摽驃杓焱"};
static const uint8_t JY_mb_biao3[]= {"表錶婊裱"};
static const uint8_t JY_mb_biao4[]= {"俵摽鰾"};
//====bian (ㄅㄧㄢ)
static const uint8_t JY_mb_bian[]= {"邊編鞭蝙稹萹砭"};
static const uint8_t JY_mb_bian3[]= {"扁貶匾辯辨褊碥惼窆編"};
static const uint8_t JY_mb_bian4[]= {"變便編扁遍徧辯辨辮釆卞弁閞汴鴘"};
//====bin (ㄅㄧㄣ)
static const uint8_t JY_mb_bin[]= {"彬賓斌頻濱檳繽瀕玢鑌豳邠份"};
static const uint8_t JY_mb_bin4[]= {"鬢殯賓儐臏擯"};
//====bing (ㄅㄧㄥ)
static const uint8_t JY_mb_bing[]= {"兵冰并檳掤絣栟"};
static const uint8_t JY_mb_bing3[]= {"丙秉餅炳柄稟迸鞞屏"};
static const uint8_t JY_mb_bing4[]= {"並病屏併并柄摒枋燹"};
//====bu (ㄅㄨ)
static const uint8_t JY_mb_bu[]= {"惚扑晡逋鈽餔"};
static const uint8_t JY_mb_bu3[]= {"補捕堡卜哺餔"};
static const uint8_t JY_mb_bu4[]= {"不部附步布佈菩薄怖埔簿埠溥鈽"};

//=====pa (ㄆㄚ)
static const uint8_t JY_mb_pa[]= {"趴葩"};
static const uint8_t JY_mb_pa2[]= {"扒杷爬琶耙跁"};
static const uint8_t JY_mb_pa4[]= {"怕帕"};
//=====po (ㄆㄛ)
static const uint8_t JY_mb_po[]= {"坡潑泼釙钋陂頗颇"};
static const uint8_t JY_mb_po2[]= {"婆皤鄱"};
static const uint8_t JY_mb_po3[]= {"叵笸"};
static const uint8_t JY_mb_po4[]= {"破迫魄朴珀粕"};
//=====pai (ㄆㄞ)
static const uint8_t JY_mb_pai[]= {"拍"};
static const uint8_t JY_mb_pai2[]= {"排俳徘棑牌簰"};
static const uint8_t JY_mb_pai3[]= {"排"};
static const uint8_t JY_mb_pai4[]= {"派湃蒎"};
//=====pei (ㄆㄟ)
static const uint8_t JY_mb_pei[]= {"呸柸胚"};
static const uint8_t JY_mb_pei2[]= {"培裴賠赔陪"};
static const uint8_t JY_mb_pei4[]= {"佩帔旆沛珮轡配霈"};
//=====pao (ㄆㄠ)
static const uint8_t JY_mb_pao[]= {"拋抛泡脬"};
static const uint8_t JY_mb_pao2[]= {"袍刨匏咆庖炰"};
static const uint8_t JY_mb_pao3[]= {"跑"};
static const uint8_t JY_mb_pao4[]= {"泡炮皰砲炮"};
//=====pou (ㄆㄡ)
static const uint8_t JY_mb_pou[]= {"剖"};
static const uint8_t JY_mb_pou2[]= {"抔掊裒"};
static const uint8_t JY_mb_pou3[]= {"掊"};
//=====pan (ㄆㄢ)
static const uint8_t JY_mb_pan[]= {"扳攀潘番眅"};
static const uint8_t JY_mb_pan2[]= {"盤盘磐幋柈槃盘磻縏蟠蹣蹒鞶"};
static const uint8_t JY_mb_pan4[]= {"判叛拚泮畔盼袢詊頖"};
//=====pen (ㄆㄣ)
static const uint8_t JY_mb_pen[]= {"噴喷歕"};
static const uint8_t JY_mb_pen2[]= {"盆湓"};
static const uint8_t JY_mb_pen4[]= {"噴喷歕"};
//=====pang (ㄆㄤ)
static const uint8_t JY_mb_pang[]= {"乓滂膀"};
static const uint8_t JY_mb_pang2[]= {"旁厖尨庬彷徬膀螃逄雱龐庞"};
static const uint8_t JY_mb_pang3[]= {"耪"};
static const uint8_t JY_mb_pang4[]= {"胖"};
//=====peng (ㄆㄥ)
static const uint8_t JY_mb_peng[]= {"烹怦抨漰砰"};
static const uint8_t JY_mb_peng2[]= {"朋堋彭棚澎硼篷膨芃蓬蟛鬅鵬鹏"};
static const uint8_t JY_mb_peng3[]= {"捧"};
static const uint8_t JY_mb_peng4[]= {"碰掽"};
//=====pi (ㄆㄧ)
static const uint8_t JY_mb_pi[]= {"匹丕伾劈坯批披狉砒秠紕纰霹駓鴄"};
static const uint8_t JY_mb_pi2[]= {"皮啤埤枇毗毘琵疲羆罴脾芘蜱裨貔郫鈹铍陂陴"};
static const uint8_t JY_mb_pi3[]= {"痞癖仳劈匹否嚭圮庀疋"};
static const uint8_t JY_mb_pi4[]= {"屁僻媲淠澼甓譬辟闢辟"};
//=====pie (ㄆㄧㄝ)
static const uint8_t JY_mb_pie[]= {"撇氕瞥"};
static const uint8_t JY_mb_pie3[]= {"撇苤"};
//=====piao (ㄆㄧㄠ)
static const uint8_t JY_mb_piao[]= {"漂飄飘螵剽"};
static const uint8_t JY_mb_piao2[]= {"嫖瓢"};
static const uint8_t JY_mb_piao3[]= {"摽殍漂瞟縹缥莩"};
static const uint8_t JY_mb_piao4[]= {"票漂驃骠"};
//=====pian (ㄆㄧㄢ)
static const uint8_t JY_mb_pian[]= {"篇翩萹扁偏扁片"};
static const uint8_t JY_mb_pian2[]= {"便楄匾楩胼蹁駢骈"};
static const uint8_t JY_mb_pian3[]= {"諞谝"};
static const uint8_t JY_mb_pian4[]= {"片騙骗"};
//=====pin (ㄆㄧㄣ)
static const uint8_t JY_mb_pin[]= {"拚拼姘"};
static const uint8_t JY_mb_pin2[]= {"蘋苹貧贫嚬嬪嫔頻频顰颦"};
static const uint8_t JY_mb_pin3[]= {"品"};
static const uint8_t JY_mb_pin4[]= {"聘牝"};
//=====ping (ㄆㄧㄥ)
static const uint8_t JY_mb_ping[]= {"乒俜娉"};
static const uint8_t JY_mb_ping2[]= {"坪平屏憑凭枰泙洴瓶缾苹萍蘋苹評评"};
//=====pu (ㄆㄨ)
static const uint8_t JY_mb_pu[]= {"撲扑仆噗扑痡鋪铺"};
static const uint8_t JY_mb_pu2[]= {"匍仆僕仆濮璞脯莆菩葡蒱蒲襆酺鏷镤"};
static const uint8_t JY_mb_pu3[]= {"譜谱圃埔普氆浦溥烳鐠镨"};
static const uint8_t JY_mb_pu4[]= {"舖铺鋪铺堡曝瀑"};

//====ma (ㄇㄚ)
static const uint8_t JY_mb_ma[]= {"媽妈嬤嬷螞蚂"};
static const uint8_t JY_mb_ma2[]= {"麻痲蟆"};
static const uint8_t JY_mb_ma3[]= {"馬马嗎吗溤瑪玛碼码螞蚂"};
static const uint8_t JY_mb_ma4[]= {"罵骂禡螞蚂"};
static const uint8_t JY_mb_ma5[]= {"嗎吗嘛蟆么"};
//====mo (ㄇㄛ)
static const uint8_t JY_mb_mo[]= {"摸"};
static const uint8_t JY_mb_mo2[]= {"磨摩魔劘嫫摸摹模糢膜藦蘑謨谟饃馍麼麽"};
static const uint8_t JY_mb_mo3[]= {"抹"};
static const uint8_t JY_mb_mo4[]= {"末莫墨默寞抹末歾歿殁沒没沫漠瘼磨秣纆茉貉貊貘鄚陌霢靺驀蓦"};
static const uint8_t JY_mb_mo5[]= {"麼麽"};
//====mo (ㄇㄜ)
static const uint8_t JY_mb_me5[]= {"乜"};
//====mai (ㄇㄞ)
static const uint8_t JY_mb_mai2[]= {"埋霾"};
static const uint8_t JY_mb_mai3[]= {"買买"};
static const uint8_t JY_mb_mai4[]= {"賣卖邁迈麥麦勱劢脈脉"};
//====mei (ㄇㄟ)
static const uint8_t JY_mb_mei2[]= {"梅堳媒嵋枚楣沒没湄煤玫瑂眉禖脢苺莓郿鋂霉黴"};
static const uint8_t JY_mb_mei3[]= {"美鎂镁媄媺每浼渼"};
static const uint8_t JY_mb_mei4[]= {"妹媚寐昧沬煝痗眛袂韎魅"};
//====mao (ㄇㄠ)
static const uint8_t JY_mb_mao[]= {"貓猫"};
static const uint8_t JY_mb_mao2[]= {"毛氂矛茅旄蝥蟊錨锚髦髳"};
static const uint8_t JY_mb_mao3[]= {"卯昴泖鉚铆"};
static const uint8_t JY_mb_mao4[]= {"冒帽貌貿贸媢懋旄楙瑁眊瞀耄芼茂袤鄮"};
//====mou (ㄇㄡ)
static const uint8_t JY_mb_mou2[]= {"謀谋侔牟眸繆缪蛑鍪麰"};
static const uint8_t JY_mb_mou3[]= {"某冇"};
//====man (ㄇㄢ)
static const uint8_t JY_mb_man[]= {"顢颟"};
static const uint8_t JY_mb_man2[]= {"蠻蛮瞞瞒埋槾蔓謾谩饅馒鰻鳗"};
static const uint8_t JY_mb_man3[]= {"滿满"};
static const uint8_t JY_mb_man4[]= {"慢曼漫熳縵缦蔓墁嫚幔謾谩鄤鏝镘鬗"};
//====men (ㄇㄣ)
static const uint8_t JY_mb_men[]= {"悶闷"};
static const uint8_t JY_mb_men2[]= {"們们捫扪璊穈鍆钔門门"};
static const uint8_t JY_mb_men4[]= {"悶闷懣懑燜焖"};
static const uint8_t JY_mb_men5[]= {"們们"};
//====mang (ㄇㄤ)
static const uint8_t JY_mb_mang2[]= {"忙盲芒茫哤尨杗氓硭邙鋩"};
static const uint8_t JY_mb_mang3[]= {"莽蟒漭"};
//====meng (ㄇㄥ)
static const uint8_t JY_mb_meng[]= {"矇蒙蒙"};
static const uint8_t JY_mb_meng2[]= {"蒙盟幪曚朦檬氓濛甍甿盟矇蒙礞艨莔萌蒙虻鸏"};
static const uint8_t JY_mb_meng3[]= {"懵猛艋蜢蠓錳锰"};
static const uint8_t JY_mb_meng4[]= {"夢梦孟"};
//====mi (ㄇㄧ)
static const uint8_t JY_mb_mi[]= {"咪瞇眯"};
static const uint8_t JY_mb_mi2[]= {"彌弥糜縻蘼謎谜迷冞瀰弥獼猕禰醚醾靡麋麛"};
static const uint8_t JY_mb_mi3[]= {"米弭敉濔眯米羋芈銤靡"};
static const uint8_t JY_mb_mi4[]= {"蜜覓觅密祕秘秘冪幂嘧塓宓密幦汨糸羃謐谧鼏"};
//====mie (ㄇㄧㄝ)
static const uint8_t JY_mb_mie[]= {"乜"};
static const uint8_t JY_mb_mie4[]= {"幭滅灭篾蔑蠛衊"};
//====miao (ㄇㄧㄠ)
static const uint8_t JY_mb_miao[]= {"喵"};
static const uint8_t JY_mb_miao2[]= {"描瞄苗"};
static const uint8_t JY_mb_miao3[]= {"秒緲缈藐邈杪淼渺眇"};
static const uint8_t JY_mb_miao4[]= {"妙廟庙玅繆缪"};
//====miu (ㄇㄧㄡ)
static const uint8_t JY_mb_miu4[]= {"謬谬"};
//====mian (ㄇㄧㄢ)
static const uint8_t JY_mb_mian2[]= {"棉眠綿绵"};
static const uint8_t JY_mb_mian3[]= {"免丏俛免冕勉娩愐沔湎眄絻緬缅靦鮸"};
static const uint8_t JY_mb_mian4[]= {"面麵面"};
//====min (ㄇㄧㄣ)
static const uint8_t JY_mb_min2[]= {"民珉玟痻緡缗苠岷"};
static const uint8_t JY_mb_min3[]= {"閩闽愍憫悯抿敏暋泯閔闵黽黾"};
//====ming (ㄇㄧㄥ)
static const uint8_t JY_mb_ming2[]= {"名明銘铭鳴鸣暝榠冥洺溟瞑茗螟鄍"};
static const uint8_t JY_mb_ming4[]= {"命"};
//====mu (ㄇㄨ)
static const uint8_t JY_mb_mu3[]= {"母牡姆姥拇牳畝亩"};
static const uint8_t JY_mb_mu4[]= {"木目募墓幕慕暮楘沐牧睦穆苜鉬钼霂"};

//====fa (ㄈㄚ)
static const uint8_t JY_mb_fa[]= {"發发"};
static const uint8_t JY_mb_fa2[]= {"乏伐筏罰罚茷閥阀"};
static const uint8_t JY_mb_fa3[]= {"法髮发"};
static const uint8_t JY_mb_fa4[]= {"琺珐"};
//====fo (ㄈㄛ)
static const uint8_t JY_mb_fo2[]= {"佛"};
//====fei (ㄈㄟ)
static const uint8_t JY_mb_fei[]= {"非飛飞妃扉緋绯菲霏馡騑啡"};
static const uint8_t JY_mb_fei2[]= {"肥淝腓"};
static const uint8_t JY_mb_fei3[]= {"匪悱斐朏棐榧篚翡蜚誹诽"};
static const uint8_t JY_mb_fei4[]= {"沸吠屝廢废怫狒疿痱砩肺芾費费鐨"};
//====fou (ㄈㄡ)
static const uint8_t JY_mb_fou2[]= {"紑芣"};
static const uint8_t JY_mb_fou3[]= {"否缶"};
//====fan (ㄈㄢ)
static const uint8_t JY_mb_fan[]= {"帆幡旛番籓繙翻"};
static const uint8_t JY_mb_fan2[]= {"凡墦樊煩烦燔璠礬矾笲繁膰蕃蘩蠜蹯釩钒鷭"};
static const uint8_t JY_mb_fan3[]= {"反返"};
static const uint8_t JY_mb_fan4[]= {"飯饭犯氾汎泛泛梵畈笵範范販贩"};
//====fen (ㄈㄣ)
static const uint8_t JY_mb_fen[]= {"分吩氛玢紛纷芬酚雰"};
static const uint8_t JY_mb_fen2[]= {"墳坟幩枌棼汾濆焚蕡蚡豶轒鼖鼢"};
static const uint8_t JY_mb_fen3[]= {"粉"};
static const uint8_t JY_mb_fen4[]= {"糞粪份僨偾分奮奋忿憤愤瀵"};
//====fang (ㄈㄤ)
static const uint8_t JY_mb_fang[]= {"方芳匚坊枋淓邡鈁钫"};
static const uint8_t JY_mb_fang2[]= {"房防妨肪魴鲂"};
static const uint8_t JY_mb_fang3[]= {"訪访仿倣仿彷昉紡纺舫髣"};
static const uint8_t JY_mb_fang4[]= {"放"};
//====feng (ㄈㄥ)
static const uint8_t JY_mb_feng[]= {"風风丰封峰崶楓枫碸灃沣烽瘋疯葑蜂豐丰酆鋒锋"};
static const uint8_t JY_mb_feng2[]= {"縫缝馮逢夆渢"};
static const uint8_t JY_mb_feng3[]= {"唪"};
static const uint8_t JY_mb_feng4[]= {"鳳凤俸奉縫缝諷讽賵"};
//====fu (ㄈㄨ)
static const uint8_t JY_mb_fu[]= {"夫伕夫孵敷枹柎砆膚肤趺跗鄜鈇麩麸"};
static const uint8_t JY_mb_fu2[]= {"伏俘刜匐孚帗幅弗彿佛怫扶拂服桴氟洑浮涪砩祓福符笰箙紱绂縛紼绋綍罘罦艴芙芾苻茀茯莩菔葍蚨蜉蝠袚袱輻辐郛韍鳧凫鵩黻"};
static const uint8_t JY_mb_fu3[]= {"輔辅俯府弣拊撫抚斧滏甫簠脯腐腑蜅郙釜頫黼"};
static const uint8_t JY_mb_fu4[]= {"付偩傅副咐婦妇富復复父祔腹蚹蝮褔覆訃讣負负賦赋賻赙赴輹阜附駙驸鮒鲋鰒鳆"};

//=====da (ㄉㄚ)
static const uint8_t JY_mb_da[]= {"噠哒答褡"};
static const uint8_t JY_mb_da2[]= {"妲怛打瘩笪答荅達达靼韃鞑"};
static const uint8_t JY_mb_da3[]= {"打"};
static const uint8_t JY_mb_da4[]= {"大"};
//=====de (ㄉㄜ)
static const uint8_t JY_mb_de2[]= {"得德"};
static const uint8_t JY_mb_de5[]= {"地底得的"};
//=====dai (ㄉㄞ)
static const uint8_t JY_mb_dai[]= {"呆待獃呆"};
static const uint8_t JY_mb_dai3[]= {"歹逮"};
static const uint8_t JY_mb_dai4[]= {"代埭大岱帶带待怠戴殆玳紿袋襶貸贷迨逮靆黛"};
//=====dei (ㄉㄟ)
static const uint8_t JY_mb_dei3[]= {"得"};
//=====dao (ㄉㄠ)
static const uint8_t JY_mb_dao[]= {"刀叨忉氘舠"};
static const uint8_t JY_mb_dao3[]= {"倒壔導导島岛搗捣擣禱祷蹈"};
static const uint8_t JY_mb_dao4[]= {"道到幬帱悼燾焘盜盗稻纛翿倒"};
//=====dou (ㄉㄡ)
static const uint8_t JY_mb_dou[]= {"兜都"};
static const uint8_t JY_mb_dou3[]= {"抖斗枓蚪陡"};
static const uint8_t JY_mb_dou4[]= {"斗痘竇窦脰荳豆讀读豆逗餖鬥斗"};
//=====dan (ㄉㄢ)
static const uint8_t JY_mb_dan[]= {"丹儋單单擔担殫殚癉瘅眈砃簞箪耽聃襌鄲郸酖"};
static const uint8_t JY_mb_dan3[]= {"膽胆撢撣掸疸紞亶"};
static const uint8_t JY_mb_dan4[]= {"蛋但僤啖啗噉彈弹憚惮擔担旦氮淡澹癉瘅石窞萏蜑誕诞霮"};
//=====dang (ㄉㄤ)
static const uint8_t JY_mb_dang[]= {"當当噹璫簹襠裆鐺铛"};
static const uint8_t JY_mb_dang3[]= {"黨党擋挡攩讜谠"};
static const uint8_t JY_mb_dang4[]= {"盪蕩荡宕擋挡檔档當当碭砀簜菪"};
//=====deng (ㄉㄥ)
static const uint8_t JY_mb_deng[]= {"燈灯登簦豋"};
static const uint8_t JY_mb_deng3[]= {"等戥"};
static const uint8_t JY_mb_deng4[]= {"蹬凳澄瞪磴鄧邓鐙镫"};
//=====di  (ㄉㄧ)
static const uint8_t JY_mb_di[]= {"低堤提氐滴羝鏑镝隄堤鞮"};
static const uint8_t JY_mb_di2[]= {"嫡敵敌滌涤狄的笛篴糴籴翟荻蔋覿觌蹢迪靮"};
static const uint8_t JY_mb_di3[]= {"底呧坻弤抵柢氐牴砥菧觝詆诋邸"};
static const uint8_t JY_mb_di4[]= {"地娣帝弟杕棣珶的睇碲禘第締缔蒂蔕蝃螮諦谛踶遞递遰"};
//=====die (ㄉㄧㄝ)
static const uint8_t JY_mb_die[]= {"爹跌"};
static const uint8_t JY_mb_die2[]= {"疊叠咥喋垤堞牒瓞碟絰蜨蝶諜谍蹀迭"};
//=====diao (ㄉㄧㄠ)
static const uint8_t JY_mb_diao3[]= {"屌"};
static const uint8_t JY_mb_diao4[]= {"吊弔吊掉窵蓧藋調调釣钓銚铫"};
//=====diu (ㄉㄧㄡ)
static const uint8_t JY_mb_diu[]= {"丟丢銩铥"};
//=====dian (ㄉㄧㄢ)
static const uint8_t JY_mb_dian[]= {"顛颠傎巔巅掂滇瘨癲癫"};
static const uint8_t JY_mb_dian3[]= {"典碘點点"};
static const uint8_t JY_mb_dian4[]= {"店殿佃坫墊垫奠淀澱淀玷甸痁癜磹簟鈿钿阽電电靛"};
//=====ding (ㄉㄧㄥ)
static const uint8_t JY_mb_ding[]= {"丁仃叮玎疔盯釘钉"};
static const uint8_t JY_mb_ding3[]= {"頂顶鼎酊"};
static const uint8_t JY_mb_ding4[]= {"定訂订椗碇鋌铤錠锭啶"};
//=====du (ㄉㄨ)
static const uint8_t JY_mb_du[]= {"嘟督都闍"};
static const uint8_t JY_mb_du2[]= {"毒獨独櫝椟瀆渎牘牍犢犊碡讀读讟韣髑黷黩"};
static const uint8_t JY_mb_du3[]= {"賭赌堵睹篤笃肚"};
static const uint8_t JY_mb_du4[]= {"杜妒度渡肚蠹鍍镀"};
//=====duo (ㄉㄨㄛ)
static const uint8_t JY_mb_duo[]= {"多哆"};
static const uint8_t JY_mb_duo2[]= {"奪夺鐸铎"};
static const uint8_t JY_mb_duo3[]= {"朵躲嚲垛埵"};
static const uint8_t JY_mb_duo4[]= {"剁垛墮堕嶞惰柮舵跺馱驮"};
//=====dui (ㄉㄨㄟ)
static const uint8_t JY_mb_dui[]= {"堆"};
static const uint8_t JY_mb_dui4[]= {"兌兑對对憝懟怼碓譈隊队"};
//=====duan (ㄉㄨㄢ)
static const uint8_t JY_mb_duan[]= {"端"};
static const uint8_t JY_mb_duan3[]= {"短"};
static const uint8_t JY_mb_duan4[]= {"斷断椴段毈碫籪簖緞缎腶鍛锻"};
//=====dun (ㄉㄨㄣ)
static const uint8_t JY_mb_dun[]= {"蹲噸吨墩惇敦敦礅鐓镦"};
static const uint8_t JY_mb_dun3[]= {"盹躉趸"};
static const uint8_t JY_mb_dun4[]= {"盾噸吨囤沌炖燉炖遁遯鈍钝頓顿"};
//=====dong (ㄉㄨㄥ)
static const uint8_t JY_mb_dong[]= {"冬咚東东氡苳蝀鶇鸫鼕"};
static const uint8_t JY_mb_dong3[]= {"董懂"};
static const uint8_t JY_mb_dong4[]= {"動动凍冻恫棟栋洞胴侗"};

//====ta (ㄊㄚ)
static const uint8_t JY_mb_ta[]= {"他塌她它牠鉈铊"};
static const uint8_t JY_mb_ta3[]= {"塔獺獭"};
static const uint8_t JY_mb_ta4[]= {"踏蹋遝嗒嚃拓搨撻挞榻沓漯錔鎉闒闥闼"};
//====te (ㄊㄜ)
static const uint8_t JY_mb_te2[]    ={"特"};
static const uint8_t JY_mb_te4[]    ={"忑忒慝特犆鋱铽"};
//====tai (ㄊㄞ)
static const uint8_t JY_mb_tai[]= {"胎苔"};
static const uint8_t JY_mb_tai2[]= {"台擡抬檯炱臺苔薹邰颱駘骀鮐鲐"};
static const uint8_t JY_mb_tai4[]= {"太態态汰泰鈦钛肽"};
//====tao (ㄊㄠ)
static const uint8_t JY_mb_tao[]= {"掏搯掏滔濤涛叨弢慆絛绦縚韜韬饕"};
static const uint8_t JY_mb_tao2[]= {"桃逃陶咷啕檮洮淘綯萄鞀"};
static const uint8_t JY_mb_tao3[]= {"討讨"};
static const uint8_t JY_mb_tao4[]= {"套"};
//====tou (ㄊㄡ)
static const uint8_t JY_mb_tou[]= {"偷媮"};
static const uint8_t JY_mb_tou2[]= {"投牏頭头骰"};
static const uint8_t JY_mb_tou3[]= {"妵斢黈"};
static const uint8_t JY_mb_tou4[]= {"透"};
//====tan (ㄊㄢ)
static const uint8_t JY_mb_tan[]= {"坍攤摊灘滩癱瘫貪贪"};
static const uint8_t JY_mb_tan2[]= {"壇坛彈弹曇昙檀潭澹痰罈覃談谈譚谭郯錟锬鐔镡餤"};
static const uint8_t JY_mb_tan3[]= {"坦忐毯禫菼袒襢贉醓嗿"};
static const uint8_t JY_mb_tan4[]= {"探歎叹炭碳嘆叹"};
//====tang (ㄊㄤ)
static const uint8_t JY_mb_tang[]= {"湯汤羰蹚鏜镗"};
static const uint8_t JY_mb_tang2[]= {"唐堂塘搪棠溏瑭糖膛螗螳醣"};
static const uint8_t JY_mb_tang3[]= {"倘儻傥帑惝淌躺"};
static const uint8_t JY_mb_tang4[]= {"燙烫趟"};
//====teng (ㄊㄥ)
static const uint8_t JY_mb_teng2[]  ={"滕疼籐縢藤螣謄誊騰腾"};
//====ti (ㄊㄧ)
static const uint8_t JY_mb_ti[]= {"剔梯踢銻锑"};
static const uint8_t JY_mb_ti2[]= {"啼提禔稊綈绨緹缇荑蹄題题騠鵜鹈"};
static const uint8_t JY_mb_ti3[]= {"體体"};
static const uint8_t JY_mb_ti4[]= {"替剃嚏屜屉悌惕揥殢涕籊薙剃趯逖倜"};
//====tie (ㄊㄧㄝ)
static const uint8_t JY_mb_tie[]= {"貼贴帖怗"};
static const uint8_t JY_mb_tie3[]= {"鐵铁驖帖"};
static const uint8_t JY_mb_tie4[]= {"帖餮"};
//====tiao (ㄊㄧㄠ)
static const uint8_t JY_mb_tiao[]= {"挑祧恌"};
static const uint8_t JY_mb_tiao2[]= {"條条調调迢笤苕蜩鞗髫鰷鲦齠龆岧"};
static const uint8_t JY_mb_tiao3[]= {"窕挑"};
static const uint8_t JY_mb_tiao4[]= {"跳朓眺糶覜"};
//====tian (ㄊㄧㄢ)
static const uint8_t JY_mb_tian[]= {"天添"};
static const uint8_t JY_mb_tian2[]= {"填甜田畋窴菾鈿钿闐阗"};
static const uint8_t JY_mb_tian3[]= {"舔忝殄淟腆餂"};
static const uint8_t JY_mb_tian4[]= {"瑱"};
//====ting(ㄊㄧㄥ)
static const uint8_t JY_mb_ting[]= {"聽听廳厅桯汀烴烃綎"};
static const uint8_t JY_mb_ting2[]= {"亭停婷庭廷渟筳莛葶蜓蝏霆"};
static const uint8_t JY_mb_ting3[]= {"挺梃珽町脡艇鋌铤頲"};
static const uint8_t JY_mb_ting4[]= {"聽听"};
//====tu (ㄊㄨ)
static const uint8_t JY_mb_tu[]= {"凸禿秃突"};
static const uint8_t JY_mb_tu2[]= {"圖图塗涂屠峹徒涂瘏稌腯荼葖途酴"};
static const uint8_t JY_mb_tu3[]= {"土釷钍吐"};
static const uint8_t JY_mb_tu4[]= {"兔吐堍菟鵵"};
//====tuan (ㄊㄨㄢ)
static const uint8_t JY_mb_tuan2[]= {"團团剸摶抟漙糰团"};
static const uint8_t JY_mb_tuan3[]= {"畽"};
static const uint8_t JY_mb_tuan4[]= {"彖"};
//====tuo (ㄊㄨㄛ)
static const uint8_t JY_mb_tuo[]= {"托拖脫脱託托飥"};
static const uint8_t JY_mb_tuo2[]= {"鴕鸵佗坨柁橐沱砣紽跎酡阤陀馱驮駝驼驒鮀鼉鼍"};
static const uint8_t JY_mb_tuo3[]= {"妥庹撱橢椭"};
static const uint8_t JY_mb_tuo4[]= {"拓唾柝籜箨蘀跅"};
//====tui (ㄊㄨㄟ)
static const uint8_t JY_mb_tui[]= {"推蓷"};
static const uint8_t JY_mb_tui2[]= {"頹颓穨隤魋"};
static const uint8_t JY_mb_tui3[]= {"腿"};
static const uint8_t JY_mb_tui4[]= {"蛻蜕退駾"};
//====tun (ㄊㄨㄣ)
static const uint8_t JY_mb_tun[]= {"吞啍暾涒"};
static const uint8_t JY_mb_tun2[]= {"屯臀囤豚軘飩饨魨"};
static const uint8_t JY_mb_tun4[]= {"褪"};
//====tong (ㄊㄨㄥ)
static const uint8_t JY_mb_tong[]= {"通痌蓪"};
static const uint8_t JY_mb_tong2[]= {"同童仝佟侗僮峒彤曈朣桐潼烔瞳穜罿茼酮銅铜"};
static const uint8_t JY_mb_tong3[]= {"統统捅桶筒筩"};
static const uint8_t JY_mb_tong4[]= {"痛慟恸衕"};

//====na (ㄋㄚ)
static const uint8_t JY_mb_na2[]= {"拿拏挐"};
static const uint8_t JY_mb_na3[]= {"那哪"};
static const uint8_t JY_mb_na4[]= {"納纳那鈉钠吶呐捺肭衲軜"};
static const uint8_t JY_mb_na5[]= {"哪"};
//====ne (ㄋㄜ)
static const uint8_t JY_mb_ne4[]= {"呢"};
static const uint8_t JY_mb_ne5[]= {"呢吶呐"};
//====nai (ㄋㄞ)
static const uint8_t JY_mb_nai3[]= {"乃奶嬭氖迺"};
static const uint8_t JY_mb_nai4[]= {"奈柰耐螚褦鼐"};
//====nei (ㄋㄟ)
static const uint8_t JY_mb_nei3[]= {"餒馁哪"};
static const uint8_t JY_mb_nei4[]= {"內内氝那"};
//====nao (ㄋㄠ)
static const uint8_t JY_mb_nao2[]= {"呶撓挠猱鐃铙"};
static const uint8_t JY_mb_nao3[]= {"惱恼瑙腦脑"};
static const uint8_t JY_mb_nao4[]= {"鬧闹淖"};
//====nou (ㄋㄡ)
static const uint8_t JY_mb_nou4[]= {"獳耨"};
//====nan (ㄋㄢ)
static const uint8_t JY_mb_nan[]= {"囝囡"};
static const uint8_t JY_mb_nan2[]= {"男難难南喃暔柟楠諵奻娚枏畘莮萳"};
static const uint8_t JY_mb_nan3[]= {"戁蝻赧腩湳罱"};
static const uint8_t JY_mb_nan4[]= {"難难婻"};
//====nen (ㄋㄣ)
static const uint8_t JY_mb_nen4[]= {"嫩"};
//====nang (ㄋㄤ)
static const uint8_t JY_mb_nang2[]= {"囊"};
static const uint8_t JY_mb_nang3[]= {"曩"};
//====neng (ㄋㄥ)
static const uint8_t JY_mb_neng2[]= {"能"};
//====ni (ㄋㄧ)
static const uint8_t JY_mb_ni2[]= {"尼倪妮呢坭泥猊蜺輗郳鈮铌霓鯢鲵麑"};
static const uint8_t JY_mb_ni3[]= {"你妳昵儗擬拟旎柅禰薿"};
static const uint8_t JY_mb_ni4[]= {"匿暱逆惄昵泥溺睨膩腻衵"};
//====nie (ㄋㄧㄝ)
static const uint8_t JY_mb_nie[]= {"捏"};
static const uint8_t JY_mb_nie4[]= {"孽嚙啮囁嗫囓敜櫱涅聶聂臬臲蘗躡蹑鎳镍鑷镊闑隉陧顳颞齧"};
//====niao (ㄋㄧㄠ)
static const uint8_t JY_mb_niao3[]= {"鳥鸟嫋嬝嬲蔦茑裊袅"};
static const uint8_t JY_mb_niao4[]= {"尿溺"};
//====niu (ㄋㄧㄡ)
static const uint8_t JY_mb_niu[]= {"妞"};
static const uint8_t JY_mb_niu2[]= {"牛"};
static const uint8_t JY_mb_niu3[]= {"扭忸杻狃紐纽鈕钮"};
static const uint8_t JY_mb_niu4[]= {"拗"};
//====nian (ㄋㄧㄢ)
static const uint8_t JY_mb_nian[]= {"蔫"};
static const uint8_t JY_mb_nian2[]= {"年粘黏"};
static const uint8_t JY_mb_nian3[]= {"捻撚捻拈攆撵涊淰碾輦辇"};
static const uint8_t JY_mb_nian4[]= {"唸念廿念"};
//====nin (ㄋㄧㄣ)
static const uint8_t JY_mb_nin2[]= {"您"};
//====niang (ㄋㄧㄤ)
static const uint8_t JY_mb_niang2[] ={"娘孃"};
static const uint8_t JY_mb_niang4[] ={"釀酿"};
//====ning (ㄋㄧㄥ)
static const uint8_t JY_mb_ning2[]= {"凝寧宁嚀咛擰拧檸柠獰狞"};
static const uint8_t JY_mb_ning3[]= {"擰拧"};
static const uint8_t JY_mb_ning4[]= {"佞擰拧濘泞"};
//====nu (ㄋㄨ)
static const uint8_t JY_mb_nu2[]= {"奴孥駑驽"};
static const uint8_t JY_mb_nu3[]= {"努弩砮"};
static const uint8_t JY_mb_nu4[]= {"怒"};
//====nuo (ㄋㄨㄛ)
static const uint8_t JY_mb_nuo2[]= {"儺傩娜挪"};
static const uint8_t JY_mb_nuo4[]= {"懦搦糯諾诺"};
//====nuan (ㄋㄨㄢ)
static const uint8_t JY_mb_nuan3[]= {"暖煖餪"};
//====nong (ㄋㄨㄥ)
static const uint8_t JY_mb_nong2[]= {"濃浓儂侬噥哝穠膿脓襛農农醲"};
static const uint8_t JY_mb_nong4[]= {"弄"};
//====nv (ㄋㄩ)
static const uint8_t JY_mb_nv3[]= {"女恧籹朒衄釹"};
//====nue (ㄋㄩㄝ)
static const uint8_t JY_mb_nue4[]= {"虐瘧謔"};


//====la (ㄌㄚ)
static const uint8_t JY_mb_la[]= {"啦喇拉垃"};
static const uint8_t JY_mb_la2[]= {"剌"};
static const uint8_t JY_mb_la3[]= {"喇"};
static const uint8_t JY_mb_la4[]= {"辣剌瘌腊臘腊落蜡蠟蜡鑞鬎"};
static const uint8_t JY_mb_la5[]= {"啦藍蓝"};
//====le (ㄌㄜ)
static const uint8_t JY_mb_le4[]= {"樂乐仂勒扐泐阞鰳鳓垃"};
static const uint8_t JY_mb_le5[]= {"了"};
//====lai (ㄌㄞ)
static const uint8_t JY_mb_lai2[]= {"來来崍崃徠徕淶涞箂萊莱錸铼騋"};
static const uint8_t JY_mb_lai4[]= {"賴赖瀨濑癩癞睞睐籟籁藾賚赉"};
//====lei (ㄌㄟ)
static const uint8_t JY_mb_lei[]= {"勒"};
static const uint8_t JY_mb_lei2[]= {"雷嫘擂檑累縲缧纍罍羸虆鐳镭"};
static const uint8_t JY_mb_lei3[]= {"磊儡壘垒樏累耒蕾藟誄诔鸓"};
static const uint8_t JY_mb_lei4[]= {"類类淚泪礧累纇肋酹擂"};
//====lao (ㄌㄠ)
static const uint8_t JY_mb_lao[]= {"撈捞"};
static const uint8_t JY_mb_lao2[]= {"勞劳嘮唠嶗崂牢癆痨醪"};
static const uint8_t JY_mb_lao3[]= {"佬姥栳潦狫老轑銠铑"};
static const uint8_t JY_mb_lao4[]= {"烙嫪澇涝絡络酪"};
//====lan (ㄌㄢ)
static const uint8_t JY_mb_lan2[]= {"籃篮籣藍蓝蘭兰婪嵐岚攔拦斕斓欄栏瀾澜襤褴襴讕谰鑭镧闌阑"};
static const uint8_t JY_mb_lan3[]= {"覽览懶懒攬揽欖榄灠壈嬾"};
static const uint8_t JY_mb_lan4[]= {"濫滥爛烂"};
//====lang (ㄌㄤ)
static const uint8_t JY_mb_lang2[]= {"狼廊桹榔琅瑯琅稂筤蜋螂郎鋃锒"};
static const uint8_t JY_mb_lang3[]= {"朗烺塱朗"};
static const uint8_t JY_mb_lang4[]= {"浪埌莨蒗"};
//====leng (ㄌㄥ)
static const uint8_t JY_mb_leng2[]  ={"崚棱楞稜薐"};
static const uint8_t JY_mb_leng3[]  ={"冷"};
static const uint8_t JY_mb_leng4[]  ={"愣"};
//====li (ㄌㄧ)
static const uint8_t JY_mb_li[]= {"哩"};
static const uint8_t JY_mb_li2[]= {"離离驪骊厘喱嫠梨梩漓漦灕犁犛狸璃籬篱縭缡罹蔾藜蘺蓠蜊蠡褵貍釐麗丽黎黧"};
static const uint8_t JY_mb_li3[]= {"裡里俚哩娌悝李浬澧理禮礼蠡邐逦醴里鋰锂鯉鲤鱧鳢"};
static const uint8_t JY_mb_li4[]= {"立利力例俐儷俪勵励厲厉吏唎唳嚦呖屴慄栗戾曆历栗櫟栎櫪枥歷历沴浰溧瀝沥痢癘疠皪盭砅礪砺礫砾笠篥粒糲苙荔莉蒞莅藶苈蠣蛎詈躒跞轢轹酈郦隸隶靂雳鬁鴗麗丽"};
static const uint8_t JY_mb_li5[]= {"哩"};
//====lie (ㄌㄧㄝ)
static const uint8_t JY_mb_lie[]= {"咧"};
static const uint8_t JY_mb_lie3[]= {"咧"};
static const uint8_t JY_mb_lie4[]= {"劣冽列捩洌烈獵猎睙茢蛚裂趔躐鬣鴷"};
//====liao (ㄌㄧㄠ)
static const uint8_t JY_mb_liao[]= {"撩"};
static const uint8_t JY_mb_liao2[]= {"僚嘹嫽寥寮撩敹燎獠療疗繚缭聊膋遼辽鷯鹩"};
static const uint8_t JY_mb_liao3[]= {"了瞭了蓼釕钌"};
static const uint8_t JY_mb_liao4[]= {"廖撂料瞭了"};
//====liu (ㄌㄧㄡ)
static const uint8_t JY_mb_liu[]= {"溜"};
static const uint8_t JY_mb_liu2[]= {"劉刘懰旒榴流瀏浏琉留瘤硫鎏鎦镏鏐飀騮骝鶹"};
static const uint8_t JY_mb_liu3[]= {"柳綹绺罶"};
static const uint8_t JY_mb_liu4[]= {"六蹓遛陸陆霤餾馏"};
//====lian (ㄌㄧㄢ)
static const uint8_t JY_mb_lian2[]= {"聯联憐怜嗹奩奁帘廉漣涟濂簾蓮莲蠊褳裢連连鎌鐮镰鬑鰱鲢"};
static const uint8_t JY_mb_lian3[]= {"臉脸斂敛歛鄻"};
static const uint8_t JY_mb_lian4[]= {"戀恋鍊炼鏈链楝歛殮殓湅澰瀲潋煉炼練练"};
//====lin (ㄌㄧㄣ)
static const uint8_t JY_mb_lin2[]= {"林嶙潾燐琳璘磷粼臨临轔辚遴鄰邻霖鱗鳞麟"};
static const uint8_t JY_mb_lin3[]= {"凜凛廩廪懍懔檁檩菻"};
static const uint8_t JY_mb_lin4[]= {"吝淋膦藺蔺賃赁"};
//====liang (ㄌㄧㄤ)
static const uint8_t JY_mb_liang2[]= {"良梁樑梁涼凉粱糧粮踉量"};
static const uint8_t JY_mb_liang3[]= {"倆俩兩两啢魎魉"};
static const uint8_t JY_mb_liang4[]= {"亮喨晾諒谅踉輛辆量"};
//====ling (ㄌㄧㄥ)
static const uint8_t JY_mb_ling2[]= {"零伶齡龄鈴铃陵靈灵凌呤囹堎柃櫺棂泠淩玲瓴綾绫羚翎聆舲苓菱蔆蛉酃醽鯪鲮鴒"};
static const uint8_t JY_mb_ling3[]= {"嶺岭領领"};
static const uint8_t JY_mb_ling4[]= {"令另"};
//====lou (ㄌㄡ)
static const uint8_t JY_mb_lou[]= {"摟搂"};
static const uint8_t JY_mb_lou2[]= {"樓楼僂偻嘍喽婁娄耬耧蔞蒌螻蝼髏髅漏"};
static const uint8_t JY_mb_lou3[]= {"摟搂嶁嵝簍篓塿"};
static const uint8_t JY_mb_lou4[]= {"漏陋露瘺鏤镂"};
//====lu (ㄌㄨ)
static const uint8_t JY_mb_lu[]= {"嚕噜"};
static const uint8_t JY_mb_lu2[]= {"蘆芦爐炉壚垆廬庐櫨栌瀘泸玈盧卢籚纑罏臚胪艫舻轤轳鑪顱颅鱸鲈鸕鸬"};
static const uint8_t JY_mb_lu3[]= {"擄掳櫓橹滷卤磠艣虜虏魯鲁鹵卤"};
static const uint8_t JY_mb_lu4[]= {"路錄录陸陆露僇戮淥渌漉潞琭璐甪盝碌磟祿禄稑穋簏簬籙菉蓼蕗賂赂輅辂轆辘逯醁騄鯥鷺鹭鹿麓"};
//====luo (ㄌㄨㄛ)
static const uint8_t JY_mb_luo[]= {"囉罗捋"};
static const uint8_t JY_mb_luo2[]= {"羅罗儸囉罗玀猡籮箩蔂蘿萝螺邏逻鑼锣騾骡"};
static const uint8_t JY_mb_luo3[]= {"裸瘰砢臝蓏蠃"};
static const uint8_t JY_mb_luo4[]= {"洛落絡络咯摞濼泺犖荦珞纙雒駱骆"};
//====luan (ㄌㄨㄢ)
static const uint8_t JY_mb_luan2[]= {"鑾銮臠脔巒峦圞孌娈欒栾灤滦鸞鸾"};
static const uint8_t JY_mb_luan3[]= {"卵"};
static const uint8_t JY_mb_luan4[]= {"亂乱"};
//====lun (ㄌㄨㄣ)
static const uint8_t JY_mb_lun[]= {"掄抡"};
static const uint8_t JY_mb_lun2[]= {"輪轮侖仑倫伦圇囵崙仑掄抡淪沦綸纶論论錀"};
static const uint8_t JY_mb_lun4[]= {"論论"};
//====long (ㄌㄨㄥ)
static const uint8_t JY_mb_long[]= {"隆"};
static const uint8_t JY_mb_long2[]= {"隆龍龙嚨咙巃曨朧胧櫳栊瀧泷瓏珑癃矓胧礱砻窿籠笼聾聋蘢茏躘"};
static const uint8_t JY_mb_long3[]= {"壟垄攏拢隴陇"};
static const uint8_t JY_mb_long4[]= {"哢弄衖"};
//====lv (ㄌㄩ)
static const uint8_t JY_mb_lv2[]= {"驢郘梠"};
static const uint8_t JY_mb_lv3[]= {"呂旅履慮綠閭侶挔郘梠縷屢櫚褸鋁膂"};
static const uint8_t JY_mb_lv4[]= {"綠慮率律挔郘梠濾膂"};
//====lue (ㄌㄩㄝ)
static const uint8_t JY_mb_lue4[]= {"掠略鋝撂擽"};

//====ga (ㄍㄚ)
static const uint8_t JY_mb_ga2[]= {"嘎噶釓钆"};
static const uint8_t JY_mb_ga3[]= {"尕"};
static const uint8_t JY_mb_ga4[]= {"尬"};
//====ge (ㄍㄜ)
static const uint8_t JY_mb_ge[]= {"割咯哥圪戈擱搁歌渮牁疙紇纥胳鴿鸽"};
static const uint8_t JY_mb_ge2[]= {"格嗝塥搿膈葛蛤觡轕鎘镉閣阁閤阖隔革骼鬲"};
static const uint8_t JY_mb_ge3[]= {"葛合哿舸蓋盖"};
static const uint8_t JY_mb_ge4[]= {"個个各箇虼鉻铬"};
//====gai (ㄍㄞ)
static const uint8_t JY_mb_gai[]= {"該该垓荄賅赅陔"};
static const uint8_t JY_mb_gai3[]= {"改"};
static const uint8_t JY_mb_gai4[]= {"丐戤概溉蓋盖鈣钙"};
//====gei (ㄍㄟ)
static const uint8_t JY_mb_gei3[]= {"給给"};
//====gao (ㄍㄠ)
static const uint8_t JY_mb_gao[]= {"高糕羔槔櫜皋睾篙膏鼛"};
static const uint8_t JY_mb_gao3[]= {"搞杲槁稿縞缟鎬镐"};
static const uint8_t JY_mb_gao4[]= {"告誥诰郜鋯锆"};
//====gou (ㄍㄡ)
static const uint8_t JY_mb_gou[]= {"勾枸溝沟篝緱缑鉤钩韝"};
static const uint8_t JY_mb_gou3[]= {"狗笱苟茍岣枸"};
static const uint8_t JY_mb_gou4[]= {"夠够姤媾彀搆構构覯觏詬诟購购遘雊冓勾垢"};
//====gan (ㄍㄢ)
static const uint8_t JY_mb_gan[]= {"乾坩干杆柑泔玕甘疳竿肝酐"};
static const uint8_t JY_mb_gan3[]= {"感敢杆桿杆橄澉稈秆赶趕赶"};
static const uint8_t JY_mb_gan4[]= {"幹干旰榦淦紺绀贛赣骭"};
//====gen (ㄍㄣ)
static const uint8_t JY_mb_gen[]= {"根跟"};
static const uint8_t JY_mb_gen4[]= {"亙亘艮茛"};
//====gang (ㄍㄤ)
static const uint8_t JY_mb_gang[]= {"剛刚堽岡冈扛杠疘綱纲缸罡肛釭鋼钢"};
static const uint8_t JY_mb_gang3[]= {"港崗岗"};
static const uint8_t JY_mb_gang4[]= {"槓杠"};
//====geng (ㄍㄥ)
static const uint8_t JY_mb_geng[]= {"耕庚更浭羹賡赓鶊"};
static const uint8_t JY_mb_geng3[]= {"哽埂梗綆绠耿郠骾鯁鲠"};
static const uint8_t JY_mb_geng4[]= {"更"};
//====gu (ㄍㄨ)
static const uint8_t JY_mb_gu[]= {"估呱咕姑孤沽泒箍箛罛菇菰蛄觚軱轂毂辜酤鴣鸪"};
static const uint8_t JY_mb_gu3[]= {"古嘏扢榖榾汩滑濲牯盬瞽穀谷罟羖股臌蓇蠱蛊詁诂谷賈贾轂毂鈷钴骨鵠鹄鼓"};
static const uint8_t JY_mb_gu4[]= {"顧顾雇僱雇固堌崮故梏牿痼錮锢"};
//====gua (ㄍㄨㄚ)
static const uint8_t JY_mb_gua[]= {"刮栝瓜胍颳刮騧"};
static const uint8_t JY_mb_gua3[]= {"寡剮剐"};
static const uint8_t JY_mb_gua4[]= {"掛挂卦挂絓罣罫褂詿诖"};
//====guo (ㄍㄨㄛ)
static const uint8_t JY_mb_guo[]= {"鍋锅咼嘓堝埚崞渦涡蟈蝈郭"};
static const uint8_t JY_mb_guo2[]= {"國国幗帼摑掴聝虢馘"};
static const uint8_t JY_mb_guo3[]= {"果槨椁粿蜾裹輠"};
static const uint8_t JY_mb_guo4[]= {"過过"};
//====guai (ㄍㄨㄞ)
static const uint8_t JY_mb_guai[]= {"乖"};
static const uint8_t JY_mb_guai3[]= {"拐柺"};
static const uint8_t JY_mb_guai4[]= {"怪夬"};
//====gui (ㄍㄨㄟ)
static const uint8_t JY_mb_gui[]= {"龜龟圭傀媯妫槼歸归珪瑰皈硅茥規规邽閨闺鮭"};
static const uint8_t JY_mb_gui3[]= {"鬼詭诡軌轨佹匭匦垝姽宄晷氿癸簋"};
static const uint8_t JY_mb_gui4[]= {"貴贵跪劌刿嶡柜桂檜桧櫃柜溎炅鱖鳜"};
//====guan (ㄍㄨㄢ)
static const uint8_t JY_mb_guan[]= {"官棺關关倌冠瘝莞觀观關关鰥鳏"};
static const uint8_t JY_mb_guan3[]= {"管館馆琯痯筦莞"};
static const uint8_t JY_mb_guan4[]= {"冠慣惯摜掼灌爟瓘盥矔祼罐觀观貫贯鑵鸛鹳丱"};
//====gun (ㄍㄨㄣ)
static const uint8_t JY_mb_gun3[]= {"滾滚緄绲蔉袞衮輥辊鯀鲧"};
static const uint8_t JY_mb_gun4[]= {"棍"};
//====guang (ㄍㄨㄤ)
static const uint8_t JY_mb_guang[]= {"光桄洸珖胱"};
static const uint8_t JY_mb_guang3[]= {"廣"};
static const uint8_t JY_mb_guang4[]= {"逛桄"};
//====gong (ㄍㄨㄥ)
static const uint8_t JY_mb_gong[]= {"工弓公功宮供宫恭攻篢肱蚣觥躬魟龔龚"};
static const uint8_t JY_mb_gong3[]= {"鞏巩拱栱汞珙廾"};
static const uint8_t JY_mb_gong4[]= {"供共貢贡"};

//====ka (ㄎㄚ)
static const uint8_t JY_mb_ka[]= {"咖喀"};
static const uint8_t JY_mb_ka3[]= {"卡"};
//====ke (ㄎㄜ)
static const uint8_t JY_mb_ke[]= {"柯棵珂磕科稞窠簻苛薖蝌軻轲鈳钶頦颏顆颗髁"};
static const uint8_t JY_mb_ke2[]= {"咳殼壳頦颏"};
static const uint8_t JY_mb_ke3[]= {"可坷岢渴"};
static const uint8_t JY_mb_ke4[]= {"克刻剋克嗑客恪榼氪溘緙缂課课錁锞"};
//====kai (ㄎㄞ)
static const uint8_t JY_mb_kai[]= {"開揩开"};
static const uint8_t JY_mb_kai3[]= {"凱凯剴剀塏垲愷恺慨楷鍇锴鎧铠闓颽"};
static const uint8_t JY_mb_kai4[]= {"愾忾愒"};
//====kao (ㄎㄠ)
static const uint8_t JY_mb_kao[]= {"尻"};
static const uint8_t JY_mb_kao3[]= {"考烤拷攷栲熇"};
static const uint8_t JY_mb_kao4[]= {"靠犒焅銬铐鮳鯌"};
//====kou (ㄎㄡ)
static const uint8_t JY_mb_kou[]= {"摳抠芤"};
static const uint8_t JY_mb_kou3[]= {"口"};
static const uint8_t JY_mb_kou4[]= {"扣叩寇筘簆釦扣鷇"};
//====kan (ㄎㄢ)
static const uint8_t JY_mb_kan[]= {"刊勘堪戡看龕龛"};
static const uint8_t JY_mb_kan3[]= {"砍侃坎檻槛欿歁莰轗"};
static const uint8_t JY_mb_kan4[]= {"看瞰矙磡衎闞阚"};
//====ken (ㄎㄣ)
static const uint8_t JY_mb_ken3[]= {"肯啃墾垦懇恳齦龈"};
static const uint8_t JY_mb_ken4[]= {"掯硍"};
//====kang (ㄎㄤ)
static const uint8_t JY_mb_kang[]= {"康慷糠"};
static const uint8_t JY_mb_kang2[]= {"扛"};
static const uint8_t JY_mb_kang4[]= {"亢伉匟囥抗炕犺鈧钪"};
//====keng (ㄎㄥ)
static const uint8_t JY_mb_keng[]= {"坑牼硜硻鏗铿阬"};
//====ku (ㄎㄨ)
static const uint8_t JY_mb_ku[]= {"哭枯堀刳窟骷"};
static const uint8_t JY_mb_ku3[]= {"苦楛"};
static const uint8_t JY_mb_ku4[]= {"酷褲裤庫库矻嚳喾"};
//====kua (ㄎㄨㄚ)
static const uint8_t JY_mb_kua[]= {"誇夸姱"};
static const uint8_t JY_mb_kua3[]= {"垮"};
static const uint8_t JY_mb_kua4[]= {"挎胯跨"};
//====kuo (ㄎㄨㄛ)
static const uint8_t JY_mb_kuo4[]= {"括廓擴扩漷闊阔鞹"};
//====kuai (ㄎㄨㄞ)
static const uint8_t JY_mb_kuai[]= {"塊块"};
static const uint8_t JY_mb_kuai3[]= {"蒯"};
static const uint8_t JY_mb_kuai4[]= {"快儈侩噲哙塊块會会澮浍獪狯筷膾脍鄶郐駃鱠"};
//====kui (ㄎㄨㄟ)
static const uint8_t JY_mb_kui[]= {"虧亏盔刲巋岿悝窺窥闚"};
static const uint8_t JY_mb_kui2[]= {"葵夔奎戣揆暌睽逵鄈頯馗騤魁"};
static const uint8_t JY_mb_kui3[]= {"傀煃跬頍"};
static const uint8_t JY_mb_kui4[]= {"潰溃匱匮喟媿愧憒愦簣篑聵聩蕢蒉餽饋馈"};
//====kuan (ㄎㄨㄢ)
static const uint8_t JY_mb_kuan[]= {"寬宽髖髋"};
static const uint8_t JY_mb_kuan3[]= {"款窾梡"};
//====kun (ㄎㄨㄣ)
static const uint8_t JY_mb_kun[]= {"昆坤崑昆晜焜琨錕锟髡鯤鲲"};
static const uint8_t JY_mb_kun3[]= {"綑閫阃困悃捆梱稛"};
static const uint8_t JY_mb_kun4[]= {"困睏困"};
//====kuang (ㄎㄨㄤ)
static const uint8_t JY_mb_kuang[]= {"筐劻匡恇誆诓"};
static const uint8_t JY_mb_kuang2[]= {"狂誑诳"};
static const uint8_t JY_mb_kuang4[]= {"況况礦矿壙圹曠旷框眶絖纊纩貺贶鄺邝"};
//====kong (ㄎㄨㄥ)
static const uint8_t JY_mb_kong[]= {"空倥崆悾"};
static const uint8_t JY_mb_kong3[]= {"孔恐倥"};
static const uint8_t JY_mb_kong4[]= {"控空鞚"};

//===ha (ㄏㄚ)
static const uint8_t JY_mb_ha[]= {"哈鉿铪"};
static const uint8_t JY_mb_ha2[]= {"蛤"};
static const uint8_t JY_mb_ha4[]= {"哈"};
//===he (ㄏㄜ)
static const uint8_t JY_mb_he[]= {"呵喝訶诃"};
static const uint8_t JY_mb_he2[]= {"何劾合和曷核河涸盍盒禾紇纥翮荷覈貉郃鉌閡阂闔阖鞨頜颌鶡齕龢"};
static const uint8_t JY_mb_he4[]= {"賀贺赫鶴鹤佫和喝嗃壑暍猲翯荷"};
//===hai (ㄏㄞ)
static const uint8_t JY_mb_hai[]= {"咍咳嗨"};
static const uint8_t JY_mb_hai2[]= {"孩還还骸"};
static const uint8_t JY_mb_hai3[]= {"海胲醢"};
static const uint8_t JY_mb_hai4[]= {"亥嗐害氦駭骇"};
//===hei (ㄏㄟ)
static const uint8_t JY_mb_hei[]= {"嘿黑"};
//===hao (ㄏㄠ)
static const uint8_t JY_mb_hao[]= {"蒿嚆薅"};
static const uint8_t JY_mb_hao2[]= {"豪嗥嚎壕毫濠號号蠔貉"};
static const uint8_t JY_mb_hao3[]= {"好郝"};
static const uint8_t JY_mb_hao4[]= {"號号浩澔灝灏皓皜耗薃鄗鎬镐顥颢好昊"};
//===hou (ㄏㄡ)
static const uint8_t JY_mb_hou[]= {"齁"};
static const uint8_t JY_mb_hou2[]= {"侯喉猴瘊篌鍭餱"};
static const uint8_t JY_mb_hou3[]= {"吼"};
static const uint8_t JY_mb_hou4[]= {"後后候厚后垕堠逅郈鄇鱟鲎"};
//===han (ㄏㄢ)
static const uint8_t JY_mb_han[]= {"憨蚶酣頇顸鼾"};
static const uint8_t JY_mb_han2[]= {"函含寒涵邗邯韓韩"};
static const uint8_t JY_mb_han3[]= {"喊罕蔊"};
static const uint8_t JY_mb_han4[]= {"悍憾捍撼旱暵汗漢汉和瀚焊熯睅翰菡螒豻釬銲焊閈頷颔"};
//===hen (ㄏㄣ)
static const uint8_t JY_mb_hen2[]= {"痕"};
static const uint8_t JY_mb_hen3[]= {"很狠"};
static const uint8_t JY_mb_hen4[]= {"恨"};
//===hang (ㄏㄤ)
static const uint8_t JY_mb_hang[]= {"夯"};
static const uint8_t JY_mb_hang2[]= {"杭航吭行頏颃"};
//===heng (ㄏㄥ)
static const uint8_t JY_mb_heng[]= {"亨哼脝"};
static const uint8_t JY_mb_heng2[]= {"橫恆恒桁横珩蘅衡姮"};
static const uint8_t JY_mb_heng4[]= {"橫横"};
//===hu (ㄏㄨ)
static const uint8_t JY_mb_hu[]= {"乎呼幠忽惚欻滹膴虍虖"};
static const uint8_t JY_mb_hu2[]= {"胡壺壶囫弧搰斛槲湖狐猢瑚糊縠葫蝴衚觳餬鬍胡鶘鹕鶻鹘"};
static const uint8_t JY_mb_hu3[]= {"虎唬滸浒琥"};
static const uint8_t JY_mb_hu4[]= {"互冱岵怙戶户戽扈楛沍滬沪瓠祜笏糊護护鄠"};
//===hua (ㄏㄨㄚ)
static const uint8_t JY_mb_hua[]= {"花嘩哗華华"};
static const uint8_t JY_mb_hua2[]= {"華华划嘩哗滑猾譁哗豁鏵铧驊骅"};
static const uint8_t JY_mb_hua4[]= {"化畫画劃划嫿摦樺桦華华話话嬅"};
//===huo (ㄏㄨㄛ)
static const uint8_t JY_mb_huo[]= {"豁"};
static const uint8_t JY_mb_huo2[]= {"活和"};
static const uint8_t JY_mb_huo3[]= {"火伙夥鈥钬"};
static const uint8_t JY_mb_huo4[]= {"霍貨货和嚄惑或擭檴湱濩獲获矱禍祸穫获臛藿蠖豁鑊镬雘嗀"};
//===huai (ㄏㄨㄞ)
static const uint8_t JY_mb_huai2[]= {"淮佪徊懷怀槐踝"};
static const uint8_t JY_mb_huai4[]= {"壞坏坏"};
//===hui (ㄏㄨㄟ)
static const uint8_t JY_mb_hui[]= {"灰煇墮堕徽恢揮挥暉晖翬虺褌詼诙豗輝辉隳麾"};
static const uint8_t JY_mb_hui2[]= {"回洄痐茴蛔迴"};
static const uint8_t JY_mb_hui3[]= {"悔毀毁燬毁虺譭毁"};
static const uint8_t JY_mb_hui4[]= {"會会匯汇卉喙嘒彗彙汇恚惠慧晦濊燴烩璯穢秽篲繢缋繪绘翽蕙薈荟蟪誨诲諱讳賄贿闠"};
//===huan (ㄏㄨㄢ)
static const uint8_t JY_mb_huan[]= {"歡欢獾讙驩懽"};
static const uint8_t JY_mb_huan2[]= {"環环還还圜嬛寰桓洹澴繯缳荁萑貆鍰锾鐶闤鬟"};
static const uint8_t JY_mb_huan3[]= {"緩缓澣睆"};
static const uint8_t JY_mb_huan4[]= {"患換换喚唤奐奂宦幻擐渙涣漶煥焕瘓痪豢轘逭"};
//===hun (ㄏㄨㄣ)
static const uint8_t JY_mb_hun[]= {"婚惛昏睯葷荤閽阍"};
static const uint8_t JY_mb_hun2[]= {"渾浑琿珲餛馄餫魂楎混"};
static const uint8_t JY_mb_hun4[]= {"圂慁混溷諢诨"};
//===huang (ㄏㄨㄤ)
static const uint8_t JY_mb_huang[]= {"慌肓荒衁"};
static const uint8_t JY_mb_huang2[]= {"凰喤徨惶湟潢煌熿獚璜皇磺篁簧艎蝗蟥遑鍠隍韹鰉鳇黃黄"};
static const uint8_t JY_mb_huang3[]= {"謊谎幌怳恍晃榥滉熀"};
static const uint8_t JY_mb_huang4[]= {"晃愰晄曂輄岲"};
//===hong (ㄏㄨㄥ)
static const uint8_t JY_mb_hong[]= {"哄烘薨訇轟轰魟吽"};
static const uint8_t JY_mb_hong2[]= {"宏弘泓洪浤竑紅红紘翃虹鋐閎闳鞃鴻鸿"};
static const uint8_t JY_mb_hong3[]= {"哄嗊"};
static const uint8_t JY_mb_hong4[]= {"鬨澒蕻哄"};

//=====ji (ㄐㄧ)
static const uint8_t JY_mb_ji[]= {"基肌機机丌乩几剞勣绩唧嘰叽奇姬屐嵇幾几擊击机激犄璣玑畸畿磯矶禨稽積积笄箕績绩羇羈羁虀觭譏讥跡迹蹟迹躋跻隮雞鸡飢饥饑饥齎齏齑圾"};
static const uint8_t JY_mb_ji2[]= {"吉集籍級级亟伋佶即及堲吃姞岌庴急戢棘楫極极殛汲潗疾瘠笈蒺蕺藉襋踖蹐鈒鶺"};
static const uint8_t JY_mb_ji3[]= {"擠挤己幾几戟掎泲給给脊蟣虮踦麂几"};
static const uint8_t JY_mb_ji4[]= {"技既季妓紀纪繼继伎偈冀劑剂塈寂寄忌悸惎暨塈洎漈濟济痵瘈癠祭稷穄穧罽芰薊蓟薺荠覬觊計计記记跽際际霽霁鮆鯽鲫鰶鱭鲚"};
//=====jia (ㄐㄧㄚ)
static const uint8_t JY_mb_jia[]= {"家佳傢家加嘉夾夹挾挟枷珈痂笳耞葭袈豭跏迦鎵镓麚"};
static const uint8_t JY_mb_jia2[]= {"夾夹恝戛莢荚蛺蛱袷裌跲郟郏鋏铗頰颊"};
static const uint8_t JY_mb_jia3[]= {"假嘏岬斝椵榎檟甲瘕胛賈贾鉀钾"};
static const uint8_t JY_mb_jia4[]= {"架价假價价嫁稼駕驾"};
//=====jie (ㄐㄧㄝ)
static const uint8_t JY_mb_jie[]= {"偕喈嗟接揭湝皆結结街階阶"};
static const uint8_t JY_mb_jie2[]= {"倢偈傑杰劫劼婕孑岊幯截拮捷杰桀桔楬榤潔洁癤疖睫碣竭節节結结絜緁羯蛣袺訐讦詰诘鮚鲒"};
static const uint8_t JY_mb_jie3[]= {"姐解"};
static const uint8_t JY_mb_jie4[]= {"介借屆届戒玠界疥芥藉蚧褯解誡诫价"};
//=====jiao (ㄐㄧㄠ)
static const uint8_t JY_mb_jiao[]= {"交焦僬姣嬌娇憍教椒澆浇燋礁簥膠胶茭蕉蛟蟭跤郊驕骄鮫鲛鵁鷦鹪"};
static const uint8_t JY_mb_jiao2[]= {"嚼"};
static const uint8_t JY_mb_jiao3[]= {"角腳脚佼僥侥儌剿勦徼摷攪搅敿湫狡皎皦矯矫筊絞绞繳缴蟜鉸铰餃饺鱎"};
static const uint8_t JY_mb_jiao4[]= {"叫嘂嶠峤徼挍教斠校滘珓皭窌窖覺觉較较轎轿酵醮釂"};
//=====jiu (ㄐㄧㄡ)
static const uint8_t JY_mb_jiu[]= {"揪糾纠揫樛究鬮阄鳩鸠"};
static const uint8_t JY_mb_jiu3[]= {"久九灸玖酒韭"};
static const uint8_t JY_mb_jiu4[]= {"救舅舊旧僦咎就廄厩捄柩疚臼鷲鹫"};
//=====jian (ㄐㄧㄢ)
static const uint8_t JY_mb_jian[]= {"尖兼堅坚奸姦奸戔戋椷殲歼淺浅湔漸渐煎熸犍監监箋笺籛緘缄縑缣肩艱艰菅菺蒹蕑鑯間间韉鞯鬋鰜鰹鲣鶼鹣"};
static const uint8_t JY_mb_jian3[]= {"剪簡简儉俭囝堿戩戬揀拣撿捡柬檢检減减瞼睑筧笕繭茧翦謇譾谫趼蹇鹼硷"};
static const uint8_t JY_mb_jian4[]= {"建箭件俴健僭劍剑栫楗檻槛毽洊漸渐澗涧濺溅瀳牮珔監监瞷腱艦舰荐薦荐見见諫谏賤贱踐践鍵键鑑鑒鉴閒間间 餞饯"};
//=====jin (ㄐㄧㄣ)
static const uint8_t JY_mb_jin[]= {"金今巾斤津矜祲禁筋衿襟觔"};
static const uint8_t JY_mb_jin3[]= {"謹谨錦锦僅仅儘尽墐巹卺廑槿瑾緊紧菫"};
static const uint8_t JY_mb_jin4[]= {"禁近進进靳勁劲噤妗寖搢晉晋殣浸燼烬盡尽縉缙藎荩賮贐赆"};
//=====jing (ㄐㄧㄥ)
static const uint8_t JY_mb_jing[]= {"京精經经兢旌晶涇泾睛荊荆莖茎菁驚惊鯨鲸"};
static const uint8_t JY_mb_jing3[]= {"井景警頸颈儆剄刭憬璟璥阱"};
static const uint8_t JY_mb_jing4[]= {"敬靜静竟境徑径淨净凈净競竞凊勁劲獍痙痉脛胫逕迳鏡镜靖靚靓"};
//=====jiang (ㄐㄧㄤ)
static const uint8_t JY_mb_jiang[]= {"江僵姜將将橿殭僵漿浆疆矼茳薑姜螿豇韁"};
static const uint8_t JY_mb_jiang3[]= {"槳桨獎奖耩膙蔣蒋講讲顜"};
static const uint8_t JY_mb_jiang4[]= {"匠將将彊洚漿浆絳绛虹醬酱降"};
//=====ju (ㄐㄩ)
static const uint8_t JY_mb_ju[]= {"居俱娵拘据椐狙琚疽罝腒苴蜛裾趄車车鋦锔鋸锯雎鞠駒驹"};
static const uint8_t JY_mb_ju2[]= {"局橘菊侷局匊挶桔椈跼踘郹鋦锔"};
static const uint8_t JY_mb_ju3[]= {"舉举莒咀枸柜櫸榉沮矩筥蒟踽齟龃"};
static const uint8_t JY_mb_ju4[]= {"巨具俱倨劇剧句埧寠屨屦懼惧拒据據据炬瞿秬窶窭聚苣虡詎讵距踞遽醵鉅钜鋸锯鐻颶飓"};
//=====jue (ㄐㄩㄝ)
static const uint8_t JY_mb_jue[]= {"嗟噘撅"};
static const uint8_t JY_mb_jue2[]= {"決决訣诀絕绝倔劂厥噱嚼孓屩崛嶡戄抉掘攫橛潏爝爵獗玦玨珏璚矍腳脚臄蕝蕨覺觉角觖觼譎谲貜趹蹶躩鴃"};
//=====juan (ㄐㄩㄢ)
static const uint8_t JY_mb_juan[]= {"娟悁捐涓脧蠲鐫镌鵑鹃圈"};
static const uint8_t JY_mb_juan3[]= {"卷捲卷"};
static const uint8_t JY_mb_juan4[]= {"倦卷圈帣悁狷獧眷睊睠絹绢鄄"};
//=====jun (ㄐㄩㄣ)
static const uint8_t JY_mb_jun[]= {"君軍军囷均皸皲菌鈞钧鮶麇"};
static const uint8_t JY_mb_jun4[]= {"俊郡峻捃浚濬焌畯竣箘菌雋隽餕駿骏"};
//=====jiong (ㄐㄩㄥ)
static const uint8_t JY_mb_jiong[]= {"坰扃"};
static const uint8_t JY_mb_jiong3[]= {"窘冏泂炯煚熲絅褧迥駉"};

//====qi (ㄑㄧ)
static const uint8_t JY_mb_qi[]= {"七凄嘁妻悽凄慼戚戚期柒栖棲栖欺沏淒凄溪漆緝缉萋諆谿溪郪"};
static const uint8_t JY_mb_qi2[]= {"其旗棋齊齐圻奇娸岐崎旂歧淇琦琪畦祁祇只祈祺綦耆臍脐萁薺荠藄蘄蕲蚔蜞蠐蛴跂軝鄿錡頎颀騎骑騏骐鬐鮨鰭鳍麒亓"};
static const uint8_t JY_mb_qi3[]= {"起乞啟启屺杞棨稽綮綺绮芑豈岂"};
static const uint8_t JY_mb_qi4[]= {"器契氣气妻憩栔棄弃企气亟汔汽泣犵盵砌磧碛葺蟿訖讫迄鏚"};
//====qia (ㄑㄧㄚ)
static const uint8_t JY_mb_qia[]= {"掐"};
static const uint8_t JY_mb_qia3[]= {"卡"};
static const uint8_t JY_mb_qia4[]= {"恰洽"};
//====qie (ㄑㄧㄝ)
static const uint8_t JY_mb_qie[]= {"切"};
static const uint8_t JY_mb_qie2[]= {"茄伽"};
static const uint8_t JY_mb_qie3[]= {"且"};
static const uint8_t JY_mb_qie4[]= {"切妾怯愜惬慊挈朅竊窃篋箧趄鍥锲"};
//====qiao (ㄑㄧㄠ)
static const uint8_t JY_mb_qiao[]= {"敲磽硗蹺跷蹻鍬锹雀墝"};
static const uint8_t JY_mb_qiao2[]= {"僑侨喬乔憔樵橋桥瞧礄翹翘蕎荞譙谯趫"};
static const uint8_t JY_mb_qiao3[]= {"巧悄愀雀企"};
static const uint8_t JY_mb_qiao4[]= {"俏峭誚诮鞘譙谯翹撬殼壳竅窍"};
//====qiu (ㄑㄧㄡ)
static const uint8_t JY_mb_qiu[]= {"丘坵丘楸秋萩蚯邱鞦秋鰍鳅鶖"};
static const uint8_t JY_mb_qiu2[]= {"仇俅厹囚巰毬求泅犰球璆盚絿虯虬蝤裘觩賕赇逑遒酋銶頄鯄鼽"};
static const uint8_t JY_mb_qiu3[]= {"糗"};
//====qian (ㄑㄧㄢ)
static const uint8_t JY_mb_qian[]= {"千仟簽签籤签僉佥岍慳悭扦掔搴牽牵粁芊褰謙谦遷迁鉛铅阡韆千騫骞愆"};
static const uint8_t JY_mb_qian2[]= {"前錢钱乾媊拑掮潛潜燂箝葥虔鈐钤鉗钳黔"};
static const uint8_t JY_mb_qian3[]= {"淺浅繾缱脥譴谴遣"};
static const uint8_t JY_mb_qian4[]= {"欠倩塹堑嵌慊歉篟綪縴纤芡茜蒨"};
//====qin (ㄑㄧㄣ)
static const uint8_t JY_mb_qin[]= {"親亲欽钦侵嶔綅衾駸"};
static const uint8_t JY_mb_qin2[]= {"琴勤噙庈懃擒檎禽秦芩芹螓雂"};
static const uint8_t JY_mb_qin3[]= {"寢寝"};
static const uint8_t JY_mb_qin4[]= {"撳揿沁菣"};
//====qiang (ㄑㄧㄤ)
static const uint8_t JY_mb_qiang[]= {"槍枪嗆呛搶抢椌瑲羌腔蜣蹌跄蹡鎗枪鏘锵鏹镪"};
static const uint8_t JY_mb_qiang2[]= {"牆墙薔蔷嬙嫱廧強强彊檣樯"};
static const uint8_t JY_mb_qiang3[]= {"搶抢繈羥羟襁鏹镪彊"};
static const uint8_t JY_mb_qiang4[]= {"嗆呛熗蹌跄"};
//====qing (ㄑㄧㄥ)
static const uint8_t JY_mb_qing[]= {"青清輕轻傾倾卿圊氫氢蜻鯖鲭"};
static const uint8_t JY_mb_qing2[]= {"情擎晴檠黥勍"};
static const uint8_t JY_mb_qing3[]= {"請请頃顷廎"};
static const uint8_t JY_mb_qing4[]= {"慶庆親亲碃磬罄"};
//====qu (ㄑㄩ)
static const uint8_t JY_mb_qu[]= {"區区屈驅驱岨嶇岖敺砠祛胠蛆蛐袪詘趨趋軀躯佉凵曲"};
static const uint8_t JY_mb_qu2[]= {"渠璩瞿劬朐氍磲籧蕖蘧衢軥鴝鸲"};
static const uint8_t JY_mb_qu3[]= {"曲取娶齲龋"};
static const uint8_t JY_mb_qu4[]= {"去覷觑趣闃阒"};
//====que (ㄑㄩㄝ)
static const uint8_t JY_mb_que[]= {"炔缺闕阙"};
static const uint8_t JY_mb_que2[]= {"瘸"};
static const uint8_t JY_mb_que4[]= {"卻却愨悫搉确碏確确碻闋阕闕阙雀鵲鹊"};
//====quan (ㄑㄩㄢ)
static const uint8_t JY_mb_quan[]= {"圈悛"};
static const uint8_t JY_mb_quan2[]= {"全拳權权泉佺牷瑔痊筌荃蜷蠸詮诠跧踡輇辁醛銓铨顴颧鬈惓"};
static const uint8_t JY_mb_quan3[]= {"犬畎綣绻"};
static const uint8_t JY_mb_quan4[]= {"券勸劝"};
//====qun (ㄑㄩㄣ)
static const uint8_t JY_mb_qun[]= {"逡"};
static const uint8_t JY_mb_qun2[]= {"群裙"};
//====qiong (ㄑㄩㄥ)
static const uint8_t JY_mb_qiong[]= {"銎"};
static const uint8_t JY_mb_qiong2[]= {"瓊琼窮穷惸煢茕璚瞏穹筇蛩跫邛"};

//====xi (ㄒㄧ)
static const uint8_t JY_mb_xi[]= {"傒僖兮凞吸唏嘻夕奚嬉嶲巇希徯恓息悉惜扱昔晞晰曦析栖棲栖樨欷氥汐浠淅溪潝烯熄熙熹燨犀犧牺皙睎矽硒磎稀穸窸粞羲膝蜥螅蟋蠵西觿譆谿溪豨蹊酅醯闟鸂鼷匚"};
static const uint8_t JY_mb_xi2[]= {"媳席槢檄瘜習习蓆席褶襲袭覡觋錫锡隰霫鰼"};
static const uint8_t JY_mb_xi3[]= {"喜囍屣徙枲洗璽玺禧簁縰纚葸蓰蟢蹝"};
static const uint8_t JY_mb_xi4[]= {"係系卌呬咥夕屭戲戏扢滊潟盻禊系細细綌繫系翕肸舄虩謑郤鄎釸鑴隙餼饩鬩阋"};
//====xia (ㄒㄧㄚ)
static const uint8_t JY_mb_xia[]= {"瞎蝦虾"};
static const uint8_t JY_mb_xia2[]= {"俠侠匣峽峡暇呷柙狎狹狭瑕硤硖祫舺蕸轄辖遐霞騢黠"};
static const uint8_t JY_mb_xia4[]= {"下嚇吓夏廈厦罅"};
//====xie (ㄒㄧㄝ)
static const uint8_t JY_mb_xie[]= {"些楔歇猲蝎蠍蝎"};
static const uint8_t JY_mb_xie2[]= {"斜邪鞋諧谐協协偕勰挾挟擷撷攜携絜纈缬脅胁襭頡颉"};
static const uint8_t JY_mb_xie3[]= {"寫写血"};
static const uint8_t JY_mb_xie4[]= {"謝谢卸泄洩媟屑屧嶰廨懈械榭渫澥瀉泻瀣灺燮獬疶紲绁絏薤蟹褻亵解躞邂駴骱"};
//====xiao (ㄒㄧㄠ)
static const uint8_t JY_mb_xiao[]= {"霄削哮嘐嘵哓囂嚣宵枵梟枭歊消潚瀟潇烋痚硝簫箫綃绡蕭萧虓蛸蠨逍銷销驍骁魈鴞"};
static const uint8_t JY_mb_xiao3[]= {"小曉晓筱篠謏"};
static const uint8_t JY_mb_xiao4[]= {"效孝校笑肖傚效嘯啸恔酵"};
//====xiu (ㄒㄧㄡ)
static const uint8_t JY_mb_xiu[]= {"休修咻庥滫羞脩貅饈馐髹鵂鸺"};
static const uint8_t JY_mb_xiu3[]= {"朽宿糔"};
static const uint8_t JY_mb_xiu4[]= {"袖鏽锈嗅宿岫溴珛琇秀繡绣臭"};
//====xian (ㄒㄧㄢ)
static const uint8_t JY_mb_xian[]= {"仙僊仙先孅憸掀暹杴氙秈纖纤躚跹銛鮮鲜"};
static const uint8_t JY_mb_xian2[]= {"賢贤閑闲閒嫺鷳鹹咸咸啣衔嫌嫻娴弦涎癇痫絃弦舷諴銜衔"};
static const uint8_t JY_mb_xian3[]= {"險险顯显鮮鲜冼姺尟嶮幰燹獫猃獮玁筅蘚藓蜆蚬跣銑铣韅"};
static const uint8_t JY_mb_xian4[]= {"限陷獻献現现僩峴岘憲宪撊晛睍線线縣县羨羡腺莧苋見见豏霰餡馅"};
//====xin (ㄒㄧㄣ)
static const uint8_t JY_mb_xin[]= {"心忻新昕欣歆炘芯莘薪訢欣辛鈊鋅锌鑫馨"};
static const uint8_t JY_mb_xin2[]= {"鄩"};
static const uint8_t JY_mb_xin3[]= {"伈"};
static const uint8_t JY_mb_xin4[]= {"信囟焮舋衅釁"};
//====xiang (ㄒㄧㄤ)
static const uint8_t JY_mb_xiang[]= {"廂厢湘瓖相箱緗缃纕膷葙薌芗襄鄉乡鑲镶香驤骧"};
static const uint8_t JY_mb_xiang2[]= {"翔詳详降庠祥"};
static const uint8_t JY_mb_xiang3[]= {"享想響响餉饷饗飨饟饷鯗鲞"};
static const uint8_t JY_mb_xiang4[]= {"向像嚮向巷曏橡相蠁象項项"};
//====xing (ㄒㄧㄥ)
static const uint8_t JY_mb_xing[]= {"星猩腥興兴蛵騂惺"};
static const uint8_t JY_mb_xing2[]= {"行型形侀刑滎荥硎邢鈃鉶陘陉餳饧"};
static const uint8_t JY_mb_xing3[]= {"醒擤省"};
static const uint8_t JY_mb_xing4[]= {"倖幸姓婞幸性悻杏興兴荇莕行"};
//====xu (ㄒㄩ)
static const uint8_t JY_mb_xu[]= {"虛需須须吁噓嘘墟嬃戌旴歔盱縃繻胥虚訏鬚须"};
static const uint8_t JY_mb_xu2[]= {"徐"};
static const uint8_t JY_mb_xu3[]= {"許许冔姁栩湑糈詡诩諝醑"};
static const uint8_t JY_mb_xu4[]= {"序婿續续蓄侐勖勗卹恤恤慉敘叙旭殈漵溆畜絮緒绪芧藇藚酗魆鱮"};
//====xue (ㄒㄩㄝ)
static const uint8_t JY_mb_xue[]= {"薛靴削"};
static const uint8_t JY_mb_xue2[]= {"學学穴觷踅鷽"};
static const uint8_t JY_mb_xue3[]= {"雪鱈鳕"};
static const uint8_t JY_mb_xue4[]= {"血"};
//====xuan (ㄒㄩㄢ)
static const uint8_t JY_mb_xuan[]= {"宣儇喧嬛愃揎暄瑄禤翾萱諠喧諼谖軒轩鍹"};
static const uint8_t JY_mb_xuan2[]= {"懸悬旋漩玄璇璿"};
static const uint8_t JY_mb_xuan3[]= {"選选咺烜癬癣"};
static const uint8_t JY_mb_xuan4[]= {"炫眩旋楦泫渲眴絢绚蔙衒炫鏇镟"};
//====xun (ㄒㄩㄣ)
static const uint8_t JY_mb_xun[]= {"勛勋勳勋塤埙壎曛焄熏燻熏獯窨纁臐薰醺"};
static const uint8_t JY_mb_xun2[]= {"巡循恂旬尋寻峋橁洵潯浔灥燖珣紃荀蟳詢询郇馴驯鱘鲟噚"};
static const uint8_t JY_mb_xun4[]= {"訊讯訓训迅遜逊侚徇噀巽徇殉汛潠蕈"};
//====xiong (ㄒㄩㄥ)
static const uint8_t JY_mb_xiong[]= {"兄兇凶凶匈洶汹胸"};
static const uint8_t JY_mb_xiong2[]= {"熊雄"};
static const uint8_t JY_mb_xiong4[]= {"敻詗"};

//====zhi (ㄓ)
static const uint8_t JY_mb_zhi[]= {"之卮只搘支枝梔栀汁知祗織织肢胝脂芝蜘隻只鳷"};
static const uint8_t JY_mb_zhi2[]= {"質质值執执姪侄摭柣植樴殖直秷稙縶絷職职蘵蟄蛰跖蹠躑踯"};
static const uint8_t JY_mb_zhi3[]= {"只止指只咫址徵恉旨枳沚祇祉紙纸芷趾軹轵酯阯黹厎"};
static const uint8_t JY_mb_zhi4[]= {"至制厔峙帙幟帜彘志忮懥摯挚擲掷智桎治滯滞炙猘畤疐痔痣秩稚窒紩緻致置致蛭袟製制觶觯誌志識识豸質质贄贽跱躓踬輊轾郅銍鋕鑕陟雉騭骘鷙鸷"};
//====zha (ㄓㄚ)
static const uint8_t JY_mb_zha[]= {"渣扎柤查楂"};
static const uint8_t JY_mb_zha2[]= {"札扎札劄炸蚻鍘铡閘闸霅"};
static const uint8_t JY_mb_zha3[]= {"眨砟鮓"};
static const uint8_t JY_mb_zha4[]= {"乍吒咋搾柵栅榨溠炸痄蚱詐诈醡"};
//====zhe (ㄓㄜ)
static const uint8_t JY_mb_zhe[]= {"蜇螫遮"};
static const uint8_t JY_mb_zhe2[]= {"折摺晢慴磔蜇蟄蛰謫谪讋轍辙鮿"};
static const uint8_t JY_mb_zhe3[]= {"者赭鍺锗"};
static const uint8_t JY_mb_zhe4[]= {"這这浙蔗鷓鹧柘"};
//====zhai (ㄓㄞ)
static const uint8_t JY_mb_zhai[]= {"摘齋側侧斋"};
static const uint8_t JY_mb_zhai2[]= {"宅擇择翟"};
static const uint8_t JY_mb_zhai3[]= {"窄"};
static const uint8_t JY_mb_zhai4[]= {"債债寨瘵砦祭"};
//====zhao (ㄓㄠ)
static const uint8_t JY_mb_zhao[]= {"嘲招昭朝著着釗钊駋"};
static const uint8_t JY_mb_zhao2[]= {"著着"};
static const uint8_t JY_mb_zhao3[]= {"找沼"};
static const uint8_t JY_mb_zhao4[]= {"兆召旐炤照笊罩肇詔诏趙赵"};
//====zhou (ㄓㄡ)
static const uint8_t JY_mb_zhou[]= {"周粥舟州洲啁婤盩譸賙周輈週周騆侜"};
static const uint8_t JY_mb_zhou2[]= {"妯軸轴"};
static const uint8_t JY_mb_zhou3[]= {"帚肘"};
static const uint8_t JY_mb_zhou4[]= {"冑胄咒咮宙晝昼甃皺皱籀紂纣縐绉繇胄酎驟骤"};
//====zhan (ㄓㄢ)
static const uint8_t JY_mb_zhan[]= {"沾佔占占旃氈毡沾瞻粘覘觇詹譫谵邅霑饘驙鱣鸇"};
static const uint8_t JY_mb_zhan3[]= {"展嶄崭搌斬斩樿琖皽盞盏輾辗颭"};
static const uint8_t JY_mb_zhan4[]= {"暫暂佔占戰战棧栈湛站綻绽蘸虥轏顫颤"};
//====zhen (ㄓㄣ)
static const uint8_t JY_mb_zhen[]= {"偵侦斟桭楨桢榛溱珍甄真砧碪禎祯箴胗臻葴薽貞贞針针鉆鍼针駗鱵"};
static const uint8_t JY_mb_zhen3[]= {"枕畛疹眕稹紾縝缜袗診诊軫轸鬒黰"};
static const uint8_t JY_mb_zhen4[]= {"振揕朕瑱賑赈酖鎮镇陣阵震鴆鸩"};
//====zhang (ㄓㄤ)
static const uint8_t JY_mb_zhang[]= {"張张彰暲樟漳獐璋章粻蟑鄣鱆嫜"};
static const uint8_t JY_mb_zhang3[]= {"掌漲涨長长鞝仉"};
static const uint8_t JY_mb_zhang4[]= {"丈仗嶂帳帐幛漲涨瘴脹胀賬账障"};
//====zheng (ㄓㄥ)
static const uint8_t JY_mb_zheng[]= {"蒸爭争崢峥征徵怔掙挣正烝猙狰症癥睜睁箏筝篜鉦钲錚铮"};
static const uint8_t JY_mb_zheng3[]= {"整拯"};
static const uint8_t JY_mb_zheng4[]= {"政正症幀帧掙挣證证鄭郑"};
//====zhu (ㄓㄨ)
static const uint8_t JY_mb_zhu[]= {"朱珠株櫧槠櫫橥洙瀦潴硃朱茱蛛誅诛諸诸豬猪跦邾銖铢侏"};
static const uint8_t JY_mb_zhu2[]= {"逐朮术燭烛瘃窋竹竺筑舳蓫軸轴"};
static const uint8_t JY_mb_zhu3[]= {"主囑嘱屬属拄渚煮矚瞩砫陼麈"};
static const uint8_t JY_mb_zhu4[]= {"祝住助杼柱柷注炷箸紵羜翥苧苎著着蛀註注鑄铸馵駐驻"};
//====zhua (ㄓㄨㄚ)
static const uint8_t JY_mb_zhua[]= {"抓撾挝髽"};
static const uint8_t JY_mb_zhua3[]= {"爪"};
//====zhuo (ㄓㄨㄛ)
static const uint8_t JY_mb_zhuo[]= {"捉桌棹涿"};
static const uint8_t JY_mb_zhuo2[]= {"卓啄拙擢斫斲梲椓濁浊濯灼琢禚篧茁著着蠗諑诼酌鐲镯鷟"};
//====zhuai (ㄓㄨㄞ)
static const uint8_t JY_mb_zhuai[]= {"拽"};
static const uint8_t JY_mb_zhuai3[]= {"跩"};
static const uint8_t JY_mb_zhuai4[]= {"拽"};
//====zhuang (ㄓㄨㄤ)
static const uint8_t JY_mb_zhuang[]= {"裝装妝妆莊庄樁桩庄"};
static const uint8_t JY_mb_zhuang4[]= {"壯壮狀状撞僮戇戆奘"};
//====zhui (ㄓㄨㄟ)
static const uint8_t JY_mb_zhui[]= {"追椎錐锥隹騅骓鵻"};
static const uint8_t JY_mb_zhui4[]= {"墜坠惴甀硾縋缒膇贅赘餟"};
//====zhuan (ㄓㄨㄢ)
static const uint8_t JY_mb_zhuan[]= {"專专磚砖塼耑专顓颛鱄"};
static const uint8_t JY_mb_zhuan3[]= {"轉转"};
static const uint8_t JY_mb_zhuan4[]= {"撰賺赚傳传撰瑑篆篹縳譔轉转饌馔"};
//====zhun (ㄓㄨㄣ)
static const uint8_t JY_mb_zhun[]= {"屯窀肫諄谆"};
static const uint8_t JY_mb_zhun3[]= {"准準准"};
//====zhong (ㄓㄨㄥ)
static const uint8_t JY_mb_zhong[]= {"中伀忠忪盅終终螽衷鍾锺鐘钟"};
static const uint8_t JY_mb_zhong3[]= {"種种冢塚冢腫肿踵"};
static const uint8_t JY_mb_zhong4[]= {"重眾众中仲狆種种"};

//====chi (ㄔ)
static const uint8_t JY_mb_chi[]= {"吃喫吃嗤媸摛瓻痴癡痴眵笞絺蚩螭郗魑鴟鸱"};
static const uint8_t JY_mb_chi2[]= {"匙坻墀弛彽持池箎篪茌蚳踟遲迟馳驰"};
static const uint8_t JY_mb_chi3[]= {"尺恥耻侈呎褫誃豉齒齿"};
static const uint8_t JY_mb_chi4[]= {"赤斥傺叱啻彳抶敕熾炽翅踅飭饬饎"};
//====cha (ㄔㄚ)
static const uint8_t JY_mb_cha[]= {"叉喳差扠插杈"};
static const uint8_t JY_mb_cha2[]= {"查叉垞察搽楂槎碴茶"};
static const uint8_t JY_mb_cha3[]= {"叉衩"};
static const uint8_t JY_mb_cha4[]= {"岔侘剎刹差杈汊衩詫诧"};
//====che (ㄔㄜ)
static const uint8_t JY_mb_che[]= {"車车硨砗"};
static const uint8_t JY_mb_che3[]= {"扯撦"};
static const uint8_t JY_mb_che4[]= {"撤澈轍辙坼屮徹彻掣"};
//====chai (ㄔㄞ)
static const uint8_t JY_mb_chai[]= {"差拆釵钗"};
static const uint8_t JY_mb_chai2[]= {"柴豺儕侪"};
static const uint8_t JY_mb_chai4[]= {"瘥蠆虿"};
//====chao (ㄔㄠ)
static const uint8_t JY_mb_chao[]= {"抄超鈔钞剿勦弨"};
static const uint8_t JY_mb_chao2[]= {"朝巢嘲晁潮"};
static const uint8_t JY_mb_chao3[]= {"吵炒"};
//====chou (ㄔㄡ)
static const uint8_t JY_mb_chou[]= {"抽搊瘳篘"};
static const uint8_t JY_mb_chou2[]= {"仇儔俦幬帱惆愁疇畴稠籌筹紬綢绸裯讎雠躊踌酬鯈"};
static const uint8_t JY_mb_chou3[]= {"丑杻瞅醜丑"};
static const uint8_t JY_mb_chou4[]= {"臭"};
//====chan (ㄔㄢ)
static const uint8_t JY_mb_chan[]= {"摻掺攙搀幨梴襜"};
static const uint8_t JY_mb_chan2[]= {"蟬蝉劖單单嬋婵孱巉廛欃毚潺澶瀍瀺磛禪禅纏缠艬蟾讒谗躔鑱饞馋"};
static const uint8_t JY_mb_chan3[]= {"產产鏟铲剷铲囅冁嵼燀蕆蒇諂谄闡阐丳"};
static const uint8_t JY_mb_chan4[]= {"懺忏儳羼"};
//====chen (ㄔㄣ)
static const uint8_t JY_mb_chen[]= {"嗔琛瞋賝郴"};
static const uint8_t JY_mb_chen2[]= {"陳陈塵尘宸忱晨沈沉煁臣諶谌辰"};
static const uint8_t JY_mb_chen4[]= {"趁稱称襯衬櫬榇疢讖谶齔龀"};
//====chang (ㄔㄤ)
static const uint8_t JY_mb_chang[]= {"昌娼倀伥猖菖錩閶阊鯧鲳鼚"};
static const uint8_t JY_mb_chang2[]= {"長长常嫦償偿嘗尝嚐尝徜腸肠萇苌裳鱨"};
static const uint8_t JY_mb_chang3[]= {"廠厂場场惝敞昶氅"};
static const uint8_t JY_mb_chang4[]= {"唱倡悵怅暢畅韔鬯"};
//====cheng (ㄔㄥ)
static const uint8_t JY_mb_cheng[]= {"稱称偁撐撑檉柽琤瞠蟶蛏赬鐺铛"};
static const uint8_t JY_mb_cheng2[]= {"誠诚成丞乘呈城埕塍宬懲惩承棖枨橙澂澄澄盛程裎郕酲"};
static const uint8_t JY_mb_cheng3[]= {"逞騁骋秤"};
static const uint8_t JY_mb_cheng4[]= {"秤稱称誠诚成"};
//====chu (ㄔㄨ)
static const uint8_t JY_mb_chu[]= {"出初樗齣"};
static const uint8_t JY_mb_chu2[]= {"廚厨櫥橱滁篨耡芻刍蒢蜍躇躕蹰鉏鋤锄除雛雏鶵"};
static const uint8_t JY_mb_chu3[]= {"楚處处儲储杵楮礎础褚"};
static const uint8_t JY_mb_chu4[]= {"畜處处觸触亍俶怵搐歜矗絀绌鄐黜"};
//====chuo (ㄔㄨㄛ)
static const uint8_t JY_mb_chuo[]= {"戳"};
static const uint8_t JY_mb_chuo4[]= {"綽绰齪龊啜婥婼惙歠輟辍逴醊辵辶"};
//====chuai (ㄔㄨㄞ)
static const uint8_t JY_mb_chuai3[]= {"揣"};
static const uint8_t JY_mb_chuai4[]= {"踹嘬"};
//====chui (ㄔㄨㄟ)
static const uint8_t JY_mb_chui[]= {"吹炊"};
static const uint8_t JY_mb_chui2[]= {"垂捶搥棰椎槌箠錘锤鎚陲圌"};
//====chuan (ㄔㄨㄢ)
static const uint8_t JY_mb_chuan[]= {"川穿氚"};
static const uint8_t JY_mb_chuan2[]= {"船傳传椽遄"};
static const uint8_t JY_mb_chuan3[]= {"喘舛荈"};
static const uint8_t JY_mb_chuan4[]= {"串玔釧钏"};
//====chuang (ㄔㄨㄤ)
static const uint8_t JY_mb_chuang[]= {"瘡疮窗幢"};
static const uint8_t JY_mb_chuang2[]= {"床幢"};
static const uint8_t JY_mb_chuang3[]= {"闖闯"};
static const uint8_t JY_mb_chuang4[]= {"創创愴怆"};
//====chun (ㄔㄨㄣ)
static const uint8_t JY_mb_chun[]= {"春杶椿輴鰆"};
static const uint8_t JY_mb_chun2[]= {"純纯脣唇椿淳漘蓴醇錞鯙鶉鹑"};
static const uint8_t JY_mb_chun3[]= {"蠢惷"};
//====chong (ㄔㄨㄥ)
static const uint8_t JY_mb_chong[]= {"充沖冲衝冲忡憧翀舂"};
static const uint8_t JY_mb_chong2[]= {"蟲虫重崇"};
static const uint8_t JY_mb_chong3[]= {"寵宠"};
static const uint8_t JY_mb_chong4[]= {"揰沖冲衝冲銃铳"};

//====shi (ㄕ)
static const uint8_t JY_mb_shi[]= {"失尸屍尸師师施溼濕湿獅狮葹蓍虱蝨詩诗邿鰤鳲"};
static const uint8_t JY_mb_shi2[]= {"什十食實实拾時时識识塒埘寔湜石祏蒔莳蝕蚀鰣鲥鼫"};
static const uint8_t JY_mb_shi3[]= {"使史始屎矢豕駛驶"};
static const uint8_t JY_mb_shi4[]= {"世事仕侍勢势嗜噬士奭室市式弒弑恃戺拭是柿栻氏澨示筮舐螫襫視视試试誓諟諡謚谥貰贳軾轼逝適适釋释鈰铈飾饰"};
//====sha (ㄕㄚ)
static const uint8_t JY_mb_sha[]= {"煞沙殺杀紗纱砂剎刹杉樧痧莎裟鎩铩鯊鲨"};
static const uint8_t JY_mb_sha2[]= {"啥"};
static const uint8_t JY_mb_sha3[]= {"傻"};
static const uint8_t JY_mb_sha4[]= {"煞嗄廈厦歃箑翣萐霎"};
//====she (ㄕㄜ)
static const uint8_t JY_mb_she[]= {"奓奢賒赊"};
static const uint8_t JY_mb_she2[]= {"甚什舌蛇闍佘折揲"};
static const uint8_t JY_mb_she3[]= {"捨舍"};
static const uint8_t JY_mb_she4[]= {"設设社舍射懾慑攝摄厙厍歙涉灄滠蔎蠂赦韘麝"};
//====shai (ㄕㄞ)
static const uint8_t JY_mb_shai[]= {"篩筛"};
static const uint8_t JY_mb_shai3[]= {"色"};
static const uint8_t JY_mb_shai4[]= {"曬晒"};
//====shei (ㄕㄟ)
static const uint8_t JY_mb_shei2[]= {"誰谁"};
//====shao (ㄕㄠ)
static const uint8_t JY_mb_shao[]= {"燒烧稍弰捎梢筲艄莦蛸"};
static const uint8_t JY_mb_shao2[]= {"勺杓芍韶"};
static const uint8_t JY_mb_shao3[]= {"少"};
static const uint8_t JY_mb_shao4[]= {"劭卲召哨少紹绍邵"};
//====shou (ㄕㄡ)
static const uint8_t JY_mb_shou[]= {"收"};
static const uint8_t JY_mb_shou3[]= {"首守手掱"};
static const uint8_t JY_mb_shou4[]= {"瘦受售壽寿授狩獸兽綬绶"};
//====shan (ㄕㄢ)
static const uint8_t JY_mb_shan[]= {"山扇搧杉潸刪删埏姍姗煽珊穇縿羶膻舢芟苫衫跚釤钐"};
static const uint8_t JY_mb_shan3[]= {"閃闪陜陝陕摻掺睒"};
static const uint8_t JY_mb_shan4[]= {"善單单墠嬗扇掞摲擅汕疝剡禪禅繕缮膳訕讪謆贍赡鄯騸骟鱔鳝"};
//====shen (ㄕㄣ)
static const uint8_t JY_mb_shen[]= {"身申深伸侁參参呻娠燊甡砷籸紳绅莘葠詵诜駪"};
static const uint8_t JY_mb_shen2[]= {"神甚什"};
static const uint8_t JY_mb_shen3[]= {"沈瀋渖哂嬸婶審审瞫矧諗谂讅"};
static const uint8_t JY_mb_shen4[]= {"甚腎肾慎椹滲渗胂脤葚蜃"};
//====shang (ㄕㄤ)
static const uint8_t JY_mb_shang[]= {"商傷伤殤殇熵觴觞"};
static const uint8_t JY_mb_shang3[]= {"賞赏晌"};
static const uint8_t JY_mb_shang4[]= {"上尚蠰"};
//====sheng (ㄕㄥ)
static const uint8_t JY_mb_sheng[]= {"聲声生甥勝胜升昇升泩牲笙胜陞升鼪"};
static const uint8_t JY_mb_sheng2[]= {"繩绳澠渑"};
static const uint8_t JY_mb_sheng3[]= {"省眚"};
static const uint8_t JY_mb_sheng4[]= {"剩盛聖圣勝胜嵊晟胜賸"};
//====shu (ㄕㄨ)
static const uint8_t JY_mb_shu[]= {"輸输舒書书疏叔姝抒摴攄摅梳樞枢殊殳毹紓纾蔬"};
static const uint8_t JY_mb_shu2[]= {"熟塾孰秫襡贖赎"};
static const uint8_t JY_mb_shu3[]= {"鼠屬属數数暑癙署薯藷蜀钃黍"};
static const uint8_t JY_mb_shu4[]= {"豎竖述數数樹树墅庶恕戍朮术束漱澍翛術术裋鉥"};
//====shua (ㄕㄨㄚ)
static const uint8_t JY_mb_shua[]= {"刷"};
static const uint8_t JY_mb_shua3[]= {"耍"};
static const uint8_t JY_mb_shua4[]= {"刷"};
//====shuo (ㄕㄨㄛ)
static const uint8_t JY_mb_shuo[]= {"說说"};
static const uint8_t JY_mb_shuo4[]= {"朔鑠铄妁搠數数槊爍烁碩硕箾蒴"};
//====shuai (ㄕㄨㄞ)
static const uint8_t JY_mb_shuai[]= {"摔衰"};
static const uint8_t JY_mb_shuai3[]= {"甩"};
static const uint8_t JY_mb_shuai4[]= {"率蟀帥帅"};
//====shui (ㄕㄨㄟ)
static const uint8_t JY_mb_shui2[]= {"誰谁"};
static const uint8_t JY_mb_shui3[]= {"水"};
static const uint8_t JY_mb_shui4[]= {"睡稅税帨說说"};
//====shuan (ㄕㄨㄢ)
static const uint8_t JY_mb_shuan[]= {"拴栓閂闩"};
static const uint8_t JY_mb_shuan4[]= {"涮"};
//====shun (ㄕㄨㄣ)
static const uint8_t JY_mb_shun3[]= {"吮楯"};
static const uint8_t JY_mb_shun4[]= {"順顺瞬舜蕣"};
//====shuang (ㄕㄨㄤ)
static const uint8_t JY_mb_shuang[]= {"雙双霜驦孀瀧泷艭"};
static const uint8_t JY_mb_shuang3[]= {"爽塽"};

//====ri (ㄖ)
static const uint8_t JY_mb_ri4[]= {"日馹"};
//====re (ㄖㄜ)
static const uint8_t JY_mb_re3[]= {"惹喏"};
static const uint8_t JY_mb_re4[]= {"熱热爇"};
//====rao (ㄖㄠ)
static const uint8_t JY_mb_rao2[]= {"饒饶嬈娆蕘荛"};
static const uint8_t JY_mb_rao3[]= {"擾扰嬈娆"};
static const uint8_t JY_mb_rao4[]= {"繞绕遶"};
//====rou (ㄖㄡ)
static const uint8_t JY_mb_rou2[]= {"揉柔禸糅葇厹蹂輮鞣"};
static const uint8_t JY_mb_rou4[]= {"肉"};
//====ran (ㄖㄢ)
static const uint8_t JY_mb_ran2[]= {"然燃髯"};
static const uint8_t JY_mb_ran3[]= {"冉染苒"};
//====ren (ㄖㄣ)
static const uint8_t JY_mb_ren2[]= {"人仁壬"};
static const uint8_t JY_mb_ren3[]= {"忍稔腍荏荵"};
static const uint8_t JY_mb_ren4[]= {"認认任刃妊牣紉纫紝葚衽訒仞軔轫韌韧鵀"};
//====rang (ㄖㄤ)
static const uint8_t JY_mb_rang2[]= {"勷瀼瓤禳穰"};
static const uint8_t JY_mb_rang3[]= {"嚷壤攘"};
static const uint8_t JY_mb_rang4[]= {"讓让"};
//====reng (ㄖㄥ)
static const uint8_t JY_mb_reng[]  ={"扔"};
static const uint8_t JY_mb_reng2[]  ={"仍礽陾"};
//====ru (ㄖㄨ)
static const uint8_t JY_mb_ru2[]= {"如儒嚅孺濡筎茹薷蠕袽襦醹銣铷鴽"};
static const uint8_t JY_mb_ru3[]= {"乳汝辱"};
static const uint8_t JY_mb_ru4[]= {"入溽縟缛蓐褥鄏"};
//====ruo (ㄖㄨㄛ)
static const uint8_t JY_mb_ruo4[]= {"若弱偌爇箬蒻鄀鶸"};
//====rui (ㄖㄨㄟ)
static const uint8_t JY_mb_rui2[]= {"緌蕤"};
static const uint8_t JY_mb_rui3[]= {"蕊橤繠"};
static const uint8_t JY_mb_rui4[]= {"瑞銳锐睿芮蚋叡枘汭"};
//====ruan (ㄖㄨㄢ)
static const uint8_t JY_mb_ruan2[]= {"壖"};
static const uint8_t JY_mb_ruan3[]= {"軟软阮朊瓀耎蝡"};
//====run (ㄖㄨㄣ)
static const uint8_t JY_mb_run2[]= {"犉"};
static const uint8_t JY_mb_run4[]= {"潤润閏闰"};
//====rong (ㄖㄨㄥ)
static const uint8_t JY_mb_rong2[]= {"容嶸嵘戎榕榮荣毧溶熔狨瑢絨绒羢肜茸蓉融蠑蝾鎔"};
static const uint8_t JY_mb_rong3[]= {"冗氄"};

//====zi (ㄗ)
static const uint8_t JY_mb_zi[]= {"茲兹資资仔吱咨姿孜孳嵫淄滋澬粢緇缁菑觜訾諮谘貲赀輜辎鄑錙锱鎡髭鯔鲻鼒齜龇"};
static const uint8_t JY_mb_zi3[]= {"子紫梓滓仔秭笫姊籽耔胏茈訾"};
static const uint8_t JY_mb_zi4[]= {"自字恣剚漬渍牸眥胾"};
static const uint8_t JY_mb_zi5[]= {"子"};
//====za (ㄗㄚ)
static const uint8_t JY_mb_za[]= {"扎紮扎匝咂"};
static const uint8_t JY_mb_za2[]= {"雜杂咱砸"};
//====ze (ㄗㄜ)
static const uint8_t JY_mb_ze2[]= {"澤泽責责則则咋嘖啧幘帻擇择笮舴萴賾赜迮"};
static const uint8_t JY_mb_ze3[]= {"怎"};
static const uint8_t JY_mb_ze4[]= {"仄昃"};
//====zai (ㄗㄞ)
static const uint8_t JY_mb_zai[]= {"災灾哉栽甾"};
static const uint8_t JY_mb_zai3[]= {"宰載载崽"};
static const uint8_t JY_mb_zai4[]= {"在再載载"};
//====zao (ㄗㄠ)
static const uint8_t JY_mb_zao[]= {"糟蹧糟遭"};
static const uint8_t JY_mb_zao2[]= {"鑿凿"};
static const uint8_t JY_mb_zao3[]= {"早棗枣澡璪藻蚤"};
static const uint8_t JY_mb_zao4[]= {"造噪慥灶燥皂譟噪躁"};
//====zei (ㄗㄟ)
static const uint8_t JY_mb_zei2[]= {"賊贼"};
//====zou (ㄗㄡ)
static const uint8_t JY_mb_zou[]= {"鄒邹陬騶驺鯫鲰掫棸緅菆諏诹謅诌"};
static const uint8_t JY_mb_zou3[]= {"走"};
static const uint8_t JY_mb_zou4[]= {"奏揍"};
//====zan (ㄗㄢ)
static const uint8_t JY_mb_zan[]= {"簪"};
static const uint8_t JY_mb_zan2[]= {"咱"};
static const uint8_t JY_mb_zan3[]= {"攢攒趲趱寁拶"};
static const uint8_t JY_mb_zan4[]= {"暫暂讚赞贊赞瓚瓒酇鏨錾"};
//====zen (ㄗㄣ)
static const uint8_t JY_mb_zen3[]= {"怎"};
static const uint8_t JY_mb_zen4[]= {"譖谮"};
//====zang (ㄗㄤ)
static const uint8_t JY_mb_zang[]= {"髒脏贓赃牂臟脏臧"};
static const uint8_t JY_mb_zang3[]= {"駔驵"};
static const uint8_t JY_mb_zang4[]= {"葬藏奘臟脏"};
//====zeng (ㄗㄥ)
static const uint8_t JY_mb_zeng[]= {"曾增憎橧矰繒缯罾"};
static const uint8_t JY_mb_zeng4[]= {"贈赠甑繒缯"};
//====zu (ㄗㄨ)
static const uint8_t JY_mb_zu[]= {"租"};
static const uint8_t JY_mb_zu2[]= {"足卒捽族鏃镞"};
static const uint8_t JY_mb_zu3[]= {"祖組组詛诅阻俎珇"};
//====zuo (ㄗㄨㄛ)
static const uint8_t JY_mb_zuo[]= {"嘬"};
static const uint8_t JY_mb_zuo2[]= {"昨捽"};
static const uint8_t JY_mb_zuo3[]= {"左佐"};
static const uint8_t JY_mb_zuo4[]= {"坐作做唑座怍柞祚胙葄酢鑿凿阼"};
//====zui (ㄗㄨㄟ)
static const uint8_t JY_mb_zui3[]= {"嘴觜"};
static const uint8_t JY_mb_zui4[]= {"最罪醉檇晬蕞"};
//====zun (ㄗㄨㄣ)
static const uint8_t JY_mb_zun[]= {"尊樽遵鐏墫壿"};
static const uint8_t JY_mb_zun3[]= {"噂撙"};
//====zuan (ㄗㄨㄢ)
static const uint8_t JY_mb_zuan[]= {"鑽钻"};
static const uint8_t JY_mb_zuan3[]= {"纂籫纘缵"};
static const uint8_t JY_mb_zuan4[]= {"賺赚鑽钻"};
//====zong (ㄗㄨㄥ)
static const uint8_t JY_mb_zong[]= {"宗棕綜综縱纵翪豵蹤踪騣鬃鬷"};
static const uint8_t JY_mb_zong3[]= {"總总傯偬摠"};
static const uint8_t JY_mb_zong4[]= {"縱纵粽從从"};

//====ci (ㄘ)
static const uint8_t JY_mb_ci[]= {"疵差"};
static const uint8_t JY_mb_ci2[]= {"慈詞词辭辞瓷磁祠茈茨雌餈"};
static const uint8_t JY_mb_ci3[]= {"此玼佌"};
static const uint8_t JY_mb_ci4[]= {"刺次賜赐伺佽莿蛓"};
//====ca (ㄘㄚ)
static const uint8_t JY_mb_ca[]= {"擦"};
//====ce (ㄘㄜ)
static const uint8_t JY_mb_ce4[]= {"策側侧冊册廁厕惻恻測测畟筴茦"};
//====cai (ㄘㄞ)
static const uint8_t JY_mb_cai[]= {"猜"};
static const uint8_t JY_mb_cai2[]= {"才材纔才裁財财"};
static const uint8_t JY_mb_cai3[]= {"採采睬綵彩跴踩采"};
static const uint8_t JY_mb_cai4[]= {"菜蔡采埰"};
//====cao (ㄘㄠ)
static const uint8_t JY_mb_cao[]= {"操糙"};
static const uint8_t JY_mb_cao2[]= {"曹槽漕艚嘈螬"};
static const uint8_t JY_mb_cao3[]= {"草懆艸"};
//====cou (ㄘㄡ)
static const uint8_t JY_mb_cou4[]= {"湊凑腠輳辏"};
//====can (ㄘㄢ)
static const uint8_t JY_mb_can[]= {"餐參参驂骖"};
static const uint8_t JY_mb_can2[]= {"慚惭殘残蚕蠶蚕"};
static const uint8_t JY_mb_can3[]= {"慘惨憯"};
static const uint8_t JY_mb_can4[]= {"燦灿孱璨粲"};
//====cen (ㄘㄣ)
static const uint8_t JY_mb_cen[]= {"嵾"};
static const uint8_t JY_mb_cen2[]= {"岑涔"};
//====cang (ㄘㄤ)
static const uint8_t JY_mb_cang[]= {"倉仓傖伧滄沧艙舱蒼苍鶬"};
static const uint8_t JY_mb_cang2[]= {"藏"};
//====ceng (ㄘㄥ)
static const uint8_t JY_mb_ceng[]= {"噌"};
static const uint8_t JY_mb_ceng2[]= {"層层嶒曾蹭鄫"};
static const uint8_t JY_mb_ceng4[]= {"蹭"};
//====cu (ㄘㄨ)
static const uint8_t JY_mb_cu[]= {"粗麤"};
static const uint8_t JY_mb_cu2[]= {"徂殂"};
static const uint8_t JY_mb_cu4[]= {"醋促卒猝瘯簇蔟踤踧蹙蹴"};
//====cuo (ㄘㄨㄛ)
static const uint8_t JY_mb_cuo[]= {"搓撮磋蹉"};
static const uint8_t JY_mb_cuo2[]= {"鹺鹾嵯痤瘥矬"};
static const uint8_t JY_mb_cuo3[]= {"瑳脞"};
static const uint8_t JY_mb_cuo4[]= {"錯错厝措剉锉挫蓌銼锉"};
//====cui (ㄘㄨㄟ)
static const uint8_t JY_mb_cui[]= {"催崔摧榱縗衰"};
static const uint8_t JY_mb_cui3[]= {"璀趡"};
static const uint8_t JY_mb_cui4[]= {"粹翠脆倅啐悴毳淬焠瘁竁膵萃"};
//====cuan (ㄘㄨㄢ)
static const uint8_t JY_mb_cuan[]= {"攛撺汆氽躥蹿"};
static const uint8_t JY_mb_cuan2[]= {"攢攒"};
static const uint8_t JY_mb_cuan4[]= {"竄窜篡爨"};
//====cun (ㄘㄨㄣ)
static const uint8_t JY_mb_cun[]= {"村皴"};
static const uint8_t JY_mb_cun2[]= {"存"};
static const uint8_t JY_mb_cun3[]= {"忖"};
static const uint8_t JY_mb_cun4[]= {"寸吋"};
//====cong (ㄘㄨㄥ)
static const uint8_t JY_mb_cong[]= {"匆囪囱樅枞璁聰聪蓯苁蔥葱鏦驄骢"};
static const uint8_t JY_mb_cong2[]= {"叢丛從从悰淙琮賨"};

//====si (ㄙ)
static const uint8_t JY_mb_si[]= {"司思偲嘶廝撕斯澌禠私絲丝緦缌罳虒螄蛳鍶锶颸鷥鸶"};
static const uint8_t JY_mb_si3[]= {"死"};
static const uint8_t JY_mb_si4[]= {"四似伺俟儩兕姒寺巳柶汜泗涘祀笥耜肆食飼饲駟驷"};
//====sa (ㄙㄚ)
static const uint8_t JY_mb_sa[]= {"撒仨"};
static const uint8_t JY_mb_sa3[]= {"洒灑撒靸"};
static const uint8_t JY_mb_sa4[]= {"薩萨颯飒卅"};
//====se (ㄙㄜ)
static const uint8_t JY_mb_se4[]= {"色瑟穡穑譅轖銫铯嗇啬塞澀涩濇圾"};
//====sai (ㄙㄞ)
static const uint8_t JY_mb_sai[]= {"塞腮鰓鳃"};
static const uint8_t JY_mb_sai4[]= {"賽赛塞"};
//====san (ㄙㄢ)
static const uint8_t JY_mb_san[]= {"三叄参毿毵鬖"};
static const uint8_t JY_mb_san3[]= {"傘伞仐散糝繖糣糂"};
static const uint8_t JY_mb_san4[]= {"散"};
//====sao (ㄙㄠ)
static const uint8_t JY_mb_sao[]= {"騷骚搔繅缫臊艘"};
static const uint8_t JY_mb_sao3[]= {"掃扫嫂"};
static const uint8_t JY_mb_sao4[]= {"臊埽掃扫"};
//====sou (ㄙㄡ)
static const uint8_t JY_mb_sou[]= {"颼飕餿馊廋搜溲獀艘蒐鄋"};
static const uint8_t JY_mb_sou3[]= {"叟嗾擻擞瞍籔藪薮謏"};
static const uint8_t JY_mb_sou4[]= {"嗽"};
//====sen (ㄙㄣ)
static const uint8_t JY_mb_sen[]= {"森"};
//====sang (ㄙㄤ)
static const uint8_t JY_mb_sang[]= {"桑喪丧"};
static const uint8_t JY_mb_sang3[]= {"嗓搡磉顙颡"};
static const uint8_t JY_mb_sang4[]= {"喪丧"};
//====seng (ㄙㄥ)
static const uint8_t JY_mb_seng[]= {"僧鬙"};
//====su (ㄙㄨ)
static const uint8_t JY_mb_su[]= {"蘇苏甦苏穌稣酥"};
static const uint8_t JY_mb_su2[]= {"俗"};
static const uint8_t JY_mb_su4[]= {"訴诉素肅肃膆嗉蔌觫謖谡速遫餗驌鱐鷫僳嗉塑夙宿愫愬榡橚泝涑溯窣簌粟"};
//====suo (ㄙㄨㄛ)
static const uint8_t JY_mb_suo[]= {"縮缩羧莎梭簑傞唆娑挲桫"};
static const uint8_t JY_mb_suo3[]= {"鎖锁索所瑣琐璅"};
//====sui (ㄙㄨㄟ)
static const uint8_t JY_mb_sui[]= {"雖虽濉眭睢荽尿"};
static const uint8_t JY_mb_sui2[]= {"綏绥隋隨随"};
static const uint8_t JY_mb_sui3[]= {"髓巂瀡"};
static const uint8_t JY_mb_sui4[]= {"歲岁檖燧璲睟碎祟穗穟繐繸襚誶谇遂邃鐩隧"};
//====suan (ㄙㄨㄢ)
static const uint8_t JY_mb_suan[]= {"酸狻痠"};
static const uint8_t JY_mb_suan4[]= {"算蒜筭"};
//====sun (ㄙㄨㄣ)
static const uint8_t JY_mb_sun[]= {"孫孙猻狲蓀荪飧"};
static const uint8_t JY_mb_sun3[]= {"損损榫筍笋簨"};
//====song (ㄙㄨㄥ)
static const uint8_t JY_mb_song[]= {"松淞菘鬆松娀崧嵩"};
static const uint8_t JY_mb_song3[]= {"聳耸竦悚慫怂"};
static const uint8_t JY_mb_song4[]= {"宋送訟讼誦诵頌颂"};

//====a (ㄚ)
static const uint8_t JY_mb_a[]= {"呵啊阿"};
static const uint8_t JY_mb_a2[]= {"嗄啊"};
static const uint8_t JY_mb_a3[]= {"啊"};
static const uint8_t JY_mb_a4[]= {"阿啊"};
static const uint8_t JY_mb_a5[]= {"阿啊"};

//====o (ㄛ)
static const uint8_t JY_mb_o[]= {"喔噢"};
static const uint8_t JY_mb_o2[]= {"哦"};
static const uint8_t JY_mb_o3[]= {"嚄"};

//====e (ㄜ)
static const uint8_t JY_mb_e[]= {"婀屙痾阿"};
static const uint8_t JY_mb_e2[]= {"俄鵝鹅俄吪哦囮娥峨莪蚵蛾訛讹鋨锇額额"};
static const uint8_t JY_mb_e3[]= {"惡恶"};
static const uint8_t JY_mb_e4[]= {"餓饿厄呃咢噩堊垩堨崿惡恶愕扼搤枙萼詻諤谔軛轭遏鄂鍔锷閼阏阨頞顎颚鱷鳄鶚鹗齶"};

//====ei (ㄝ, ㄟ)
static const uint8_t JY_mb_ei4[]={"誒欸"};

//====ai (ㄞ)
static const uint8_t JY_mb_ai[]={"哎哀唉埃挨"};
static const uint8_t JY_mb_ai2[]={"癌挨捱敳皚"};
static const uint8_t JY_mb_ai3[]={"矮藹蔼靄霭噯嗳"};
static const uint8_t JY_mb_ai4[]={"愛爱曖暧璦瑷礙碍艾鑀隘靉乂僾唉噯嗳壒"};

//====ao (ㄠ)
static const uint8_t JY_mb_ao[]= {"凹熬"};
static const uint8_t JY_mb_ao2[]= {"敖熬獒璈翱聱螯謷遨鏖鰲鳌鼇嗷廒"};
static const uint8_t JY_mb_ao3[]= {"襖袄媼媪"};
static const uint8_t JY_mb_ao4[]= {"奧奥澳傲坳墺奡嶴懊拗驁骜"};

//====ou (ㄡ)
static const uint8_t JY_mb_ou[]= {"歐欧毆殴漚沤甌瓯謳讴鷗鸥"};
static const uint8_t JY_mb_ou3[]= {"偶嘔呕耦藕"};
static const uint8_t JY_mb_ou4[]= {"漚沤"};

//====an (ㄢ)
static const uint8_t JY_mb_an[]={"安庵桉氨盦菴萻諳谙鞍媕鵪鹌"};
static const uint8_t JY_mb_an3[]={"俺銨铵"};
static const uint8_t JY_mb_an4[]={"岸按暗案犴胺豻闇暗黯"};

//====en (ㄣ)
static const uint8_t JY_mb_en[]= {"恩嗯"};
static const uint8_t JY_mb_en4[]= {"摁"};

//====ang (ㄤ)
static const uint8_t JY_mb_ang[]={"骯肮"};
static const uint8_t JY_mb_ang2[]={"昂卬"};
static const uint8_t JY_mb_ang4[]={"盎"};

//====er (ㄦ)
static const uint8_t JY_mb_er2[]= {"兒儿栭洏而耏胹輀陑鮞鲕鴯鸸"};
static const uint8_t JY_mb_er3[]= {"耳爾尔尒洱珥邇迩鉺铒餌饵駬"};
static const uint8_t JY_mb_er4[]= {"二貳贰佴刵咡樲"};
static const uint8_t JY_mb_er5[]= {"兒儿"};

//====yi (ㄧ)
static const uint8_t JY_mb_yi[]= {"一伊依咿噫壹揖欹漪猗禕繄蛜衣醫医銥铱鷖黟"};
static const uint8_t JY_mb_yi2[]= {"遺遗儀仪匜夷姨宜宧峓嶷彝怡杝柂桋椸沂洟疑痍眙移簃胰荑袲訑詒诒貤貽贻迤酏頤颐飴饴"};
static const uint8_t JY_mb_yi3[]= {"乙已以倚偯扆旖椅矣艤舣苡螘蟻蚁迤釔钇顗鳦齮"};
static const uint8_t JY_mb_yi4[]= {"意億亿乂亄亦仡佚佾刈劓勩唈囈呓圛埶埸奕屹嶧峄帟异弈弋役悒憶忆懌怿懿抑挹斁易曀杙枻檍殪毅浥溢熠熤燡異异疫瘞瘗益縊缢繹绎義义羿翊翌翳翼肄臆艗蓺薏藙藝艺蜴衣裔襼詣诣誼谊譯译議议豷軼轶逸邑鎰镒鐿镱饐驛驿"};
//====ya (ㄧㄚ)
static const uint8_t JY_mb_ya[]= {"壓压丫啞哑押椏桠鴉鸦鴨鸭"};
static const uint8_t JY_mb_ya2[]= {"牙芽涯蚜衙"};
static const uint8_t JY_mb_ya3[]= {"亞雅啞哑"};
static const uint8_t JY_mb_ya4[]= {"訝讶壓压婭娅掗揠砑軋轧迓錏亞亚"};
static const uint8_t JY_mb_ya5[]= {"呀"};
//====yo (ㄧㄛ)
static const uint8_t JY_mb_yo[]= {"唷"};
static const uint8_t JY_mb_yo5[]= {"喲哟"};
//====ye (ㄧㄝ)
static const uint8_t JY_mb_ye[]= {"噎掖耶蠮"};
static const uint8_t JY_mb_ye2[]= {"爺爷揶嘢"};
static const uint8_t JY_mb_ye3[]= {"野也冶"};
static const uint8_t JY_mb_ye4[]= {"夜業业頁页咽拽擫曄晔曳液燁烨腋葉叶謁谒鄴邺靨靥饁"};
//====yai (ㄧㄞ)
static const uint8_t JY_mb_yai2[]= {"崖"};
//====yao (ㄧㄠ)
static const uint8_t JY_mb_yao[]= {"夭么吆喓妖祅約约腰葽要邀"};
static const uint8_t JY_mb_yao2[]= {"姚傜垚堯尧嶢徭搖摇殽肴爻猺珧瑤瑶窯窑繇肴謠谣軺轺遙遥銚铫颻餚肴鰩鳐"};
static const uint8_t JY_mb_yao3[]= {"咬杳窅窈窔舀鷕"};
static const uint8_t JY_mb_yao4[]= {"要鑰钥鷂鹞藥药曜燿瘧疟耀葯"};
static const uint8_t JY_mb_yao5[]= {"哟"};
//====you (ㄧㄡ)
static const uint8_t JY_mb_you[]= {"幽悠攸呦蚴怮憂優懮忧嚘櫌纋耰瀀櫌鄾喲麀妋"};
static const uint8_t JY_mb_you2[]= {"尤斿楢油游猶犹猷由疣繇蕕莸蚰蝣訧輶逌遊游郵邮鈾铀魷鱿"};
static const uint8_t JY_mb_you3[]= {"友有懮卣槱牖羑莠酉銪铕黝"};
static const uint8_t JY_mb_you4[]= {"右又佑侑優优囿宥幼柚祐佑蚴誘诱釉鴢"};
//====yan (ㄧㄢ)
static const uint8_t JY_mb_yan[]= {"煙烟胭腌臙菸鄢醃腌閹阉閼阏燕咽奄嫣崦懨恹殷淊淹湮焉"};
static const uint8_t JY_mb_yan2[]= {"嚴严埏妍岩嵒巖岩延揅檐沿炎癌研碞筵簷檐綖蜒言郔閆闫閻阎阽顏颜鹽盐"};
static const uint8_t JY_mb_yan3[]= {"眼演琰甗罨蝘衍郾馣魘魇鰋黶鼴鼹偃儼俨兗兖剡厴厣奄巘弇扊掩揜棪沇"};
static const uint8_t JY_mb_yan4[]= {"厭厌咽唁喭嚥堰宴彥彦晏沿灩滟焰焱燄焰燕爓硯砚艷艳諺谚讌宴讞谳豔贗赝醼釅酽雁饜餍驗验鴈"};
//====yin (ㄧㄣ)
static const uint8_t JY_mb_yin[]= {"音因堙姻喑愔慇殷氤湮瘖禋絪茵裀銦铟闉陰阴駰"};
static const uint8_t JY_mb_yin2[]= {"銀银霪齦龈吟嚚垠夤寅崟淫狺誾鄞"};
static const uint8_t JY_mb_yin3[]= {"飲饮尹引殷癮瘾蚓隱隐靷"};
static const uint8_t JY_mb_yin4[]= {"印廕憖窨胤蔭荫"};
//====yang (ㄧㄤ)
static const uint8_t JY_mb_yang[]= {"央殃泱秧坱鴦鸯"};
static const uint8_t JY_mb_yang2[]= {"羊楊杨洋佯垟徉揚扬暘烊煬炀瘍疡鍚陽阳颺"};
static const uint8_t JY_mb_yang3[]= {"養养仰氧痒癢鞅"};
static const uint8_t JY_mb_yang4[]= {"樣恙漾怏瀁"};
//====ying (ㄧㄥ)
static const uint8_t JY_mb_ying[]= {"英應应嚶嘤嫈嬰婴攖撄櫻樱煐瑛瓔璎甇甖纓缨罃罌罂膺霙韺鶯莺鷹鹰鸚鹦"};
static const uint8_t JY_mb_ying2[]= {"籯盈營营瑩莹塋茔嬴楹瀅滢瀛瀠潆熒荧縈萦螢萤蠅蝇謍贏赢迎"};
static const uint8_t JY_mb_ying3[]= {"影潁癭瘿穎颖郢"};
static const uint8_t JY_mb_ying4[]= {"映硬應应媵"};

//====wu (ㄨ)
static const uint8_t JY_mb_wu[]= {"屋嗚呜圬巫杇汙污污洿烏乌誣诬鄔邬鎢钨"};
static const uint8_t JY_mb_wu2[]= {"吳吴吾峿梧毋無无蕪芜蜈郚鋘鋙鼯"};
static const uint8_t JY_mb_wu3[]= {"五仵伍侮午嫵妩廡庑忤憮怃摀捂武甒碔舞鵡鹉"};
static const uint8_t JY_mb_wu4[]= {"務务勿兀卼塢坞婺寤屼悟惡恶戊扤晤杌沕物痦芴誤误遻鋈霧雾騖骛鶩"};
//====wa (ㄨㄚ)
static const uint8_t JY_mb_wa[]= {"挖蛙哇媧娲洼溛穵窊窪洼"};
static const uint8_t JY_mb_wa2[]= {"娃"};
static const uint8_t JY_mb_wa3[]= {"瓦佤"};
static const uint8_t JY_mb_wa4[]= {"襪袜膃腽"};
//====wo (ㄨㄛ)
static const uint8_t JY_mb_wo[]= {"窩窝倭渦涡萵莴喔挝猧"};
static const uint8_t JY_mb_wo3[]= {"我婐捰"};
static const uint8_t JY_mb_wo4[]= {"臥卧齷龌幄握斡沃涴渥喔焥偓濣鍄艧"};
//====wai (ㄨㄞ)
static const uint8_t JY_mb_wai[]= {"歪"};
static const uint8_t JY_mb_wai4[]= {"外"};
//====wei (ㄨㄟ)
static const uint8_t JY_mb_wei[]= {"威危委溾烓煨萎葳偎隈"};
static const uint8_t JY_mb_wei2[]= {"維维唯囗圍围微薇圩嵬帷幃帏惟桅湋濰潍為为違违鍏闈闱韋韦"};
static const uint8_t JY_mb_wei3[]= {"偉伟亹偽伪唯委娓尾洧煒炜猥瑋玮痿磈緯纬萎葦苇薳諉诿隗韙韪頠骫鮪鲔"};
static const uint8_t JY_mb_wei4[]= {"胃魏位味喂尉慰未渭為为煟畏磑罻蔚薉蝟衛卫褽謂谓躗霨餵喂"};
//====wan (ㄨㄢ)
static const uint8_t JY_mb_wan[]= {"彎弯灣湾蜿豌剜"};
static const uint8_t JY_mb_wan2[]= {"丸完汍烷玩紈纨芄頑顽"};
static const uint8_t JY_mb_wan3[]= {"晚娩婉宛挽澣琬畹皖碗綰绾莞菀輓挽"};
static const uint8_t JY_mb_wan4[]= {"萬万卍玩翫腕"};
//====wen (ㄨㄣ)
static const uint8_t JY_mb_wen[]= {"溫温瘟轀"};
static const uint8_t JY_mb_wen2[]= {"文玟紋纹聞闻蚊閺雯"};
static const uint8_t JY_mb_wen3[]= {"吻刎穩稳"};
static const uint8_t JY_mb_wen4[]= {"問问抆搵汶紊"};
//====wang (ㄨㄤ)
static const uint8_t JY_mb_wang[]= {"汪尪尢"};
static const uint8_t JY_mb_wang2[]= {"王亡"};
static const uint8_t JY_mb_wang3[]= {"網网往惘枉罔輞辋魍"};
static const uint8_t JY_mb_wang4[]= {"忘旺往望朢妄迋"};
//====weng (ㄨㄥ)
static const uint8_t JY_mb_weng[]= {"翁嗡螉鶲"};
static const uint8_t JY_mb_weng3[]= {"蓊滃"};
static const uint8_t JY_mb_weng4[]= {"甕罋瓮"};

//====yu (ㄩ)
static const uint8_t JY_mb_yu[]= {"淤箊紆纡迂"};
static const uint8_t JY_mb_yu2[]= {"魚鱼余予于俞妤娛娱嵎愉愚揄於于旟杅楰榆歟欤渝漁渔狳瑜璵畬畲盂禺窬竽羭腴臾舁與与萸蕍虞蝓褕覦觎諛谀踰逾輿舆轝逾邘隃隅雩餘馀"};
static const uint8_t JY_mb_yu3[]= {"雨羽予俁俣傴伛噳圄宇寙嶼屿庾敔楀瑀瘐禹窳與与語语齬龉"};
static const uint8_t JY_mb_yu4[]= {"玉寓峪彧御愈慾欲昱棜棫吁喻域堉嫗妪欲毓浴淢澦煜熨燏燠獄狱瘉癒愈矞禦御籲吁緎繘罭聿育與与芋蔚蕷蓣薁蜮裕語语諭谕譽誉谷豫遇遹郁鈺钰閾阈隩預预飫饫馭驭驈鬱郁鬻魊鴥鵒鹆鷸鹬"};
//====yue (ㄩㄝ)
static const uint8_t JY_mb_yue[]= {"約约曰"};
static const uint8_t JY_mb_yue4[]= {"岳嶽岳刖悅悦月樂乐樾瀹爚玥礿禴籥粵粤越趯躍跃軏鉞钺鑰钥閱阅鸑龠"};
//====yuan (ㄩㄢ)
static const uint8_t JY_mb_yuan[]= {"冤淵渊眢蜎鳶鸢鴛鸳鵷"};
static const uint8_t JY_mb_yuan2[]= {"元原員员園园圓圆圜垣媛嫄援櫞橼沅湲源爰猭猿笎緣缘羱芫蝝蝯螈袁轅辕邧騵黿鼋"};
static const uint8_t JY_mb_yuan3[]= {"遠远"};
static const uint8_t JY_mb_yuan4[]= {"院願愿媛怨愿掾瑗苑"};
//====yun (ㄩㄣ)
static const uint8_t JY_mb_yun[]= {"暈晕氳氲縕贇"};
static const uint8_t JY_mb_yun2[]= {"雲云勻匀沄澐熉畇筠篔紜纭耘芸蕓鄖郧鋆"};
static const uint8_t JY_mb_yun3[]= {"允殞殒狁隕陨霣"};
static const uint8_t JY_mb_yun4[]= {"運运韻韵孕惲恽慍愠暈晕熅熨縕薀蘊蕴鄆郓醞酝韞韫"};
//====yong (ㄩㄥ)
static const uint8_t JY_mb_yong[]= {"庸傭佣嗈墉壅廱慵擁拥灉癰痈邕鄘鏞镛雍雝饔"};
static const uint8_t JY_mb_yong2[]= {"顒喁"};
static const uint8_t JY_mb_yong3[]= {"勇俑埇恿永泳涌湧涌甬蛹詠咏踊踴踊"};
static const uint8_t JY_mb_yong4[]= {"用佣"};
static const uint8_t JY_mb_space[] ={""};

#define sizeofB 56  //ㄅ
#define sizeofP 59  //ㄆ
#define sizeofM 57  //ㄇ
#define sizeofF 31	//ㄈ
#define sizeofD 59  //ㄉ
#define sizeofT 66  //ㄊ
#define sizeofN 53  //ㄋ
#define sizeofL 80  //ㄌ
#define sizeofG 55  //ㄍ
#define sizeofK 49  //ㄎ
#define sizeofH 63  //ㄏ
#define sizeofJ 44	//ㄐ
#define sizeofQ 49	//ㄑ
#define sizeofX 51	//ㄒ
#define sizeofZH 59  //ㄓ
#define sizeofCH 61  //ㄔ
#define sizeofSH 58  //ㄕ
#define sizeofR 31  //ㄖ
#define sizeofZ 48  //ㄗ
#define sizeofC 44  //ㄘ
#define sizeofS 39  //ㄙ
#define sizeofA 5  //ㄚ
#define sizeofO 3  //ㄛ
#define sizeofE 4  //ㄜ
#define sizeofEI 1  //ㄝ,ㄟ
#define sizeofAI 4  //ㄞ
#define sizeofAO 4  //ㄠ
#define sizeofOU 3  //ㄡ
#define sizeofAN 3  //ㄢ
#define sizeofEN 2  //ㄣ
#define sizeofANG 3  //ㄤ
#define sizeofER 4  //ㄦ
#define sizeofY 41  //ㄧ
#define sizeofW 32  //ㄨ
#define sizeofYU 18  //ㄩ


//注音符號與標準鍵盤對照表
// ㄅ:1  ㄆ:Q  ㄇ:A  ㄈ:Z  ㄉ:2  ㄊ:W  ㄋ:S  ㄌ:X
// ㄍ:E  ㄎ:D  ㄏ:C  ㄐ:R  ㄑ:F  ㄒ:V  ㄓ:5  ㄔ:T
// ㄕ:G  ㄖ:B  ㄗ:Y  ㄘ:H  ㄙ:N  ㄧ:U  ㄨ:J  ㄩ:M
// ㄚ 8  ㄛ:I  ㄜ:K  ㄝ:o  ㄞ:9  ㄟ:O  ㄠ:L  ㄡ:.
// ㄢ 0  ㄣ:P  ㄤ:;  ㄥ:/  ㄦ:-
// 二聲: 6  三聲: 3  四聲: 4  入聲: 7

// ㄅ
static const uint8_t JY_index_b[][6] = {
    {"8    "},
    {"86   "},
    {"83   "},
    {"84   "},
    {"87   "},
    {"i    "},
    {"i6   "},
    {"i3   "},
    {"i4   "},
    {"i7   "},
    {"9    "},
    {"96   "},
    {"93   "},
    {"94   "},
    {"o    "},
    {"o3   "},
    {"o4   "},
    {"l    "},
    {"l6   "},
    {"l3   "},
    {"l4   "},
    {"0    "},
    {"03   "},
    {"04   "},
    {"p    "},
    {"p3   "},
    {"p4   "},
    {";    "},
    {";3   "},
    {";4   "},
    {"/    "},
    {"/6   "},
    {"/3   "},
    {"/4   "},
    {"u    "},
    {"u6   "},
    {"u3   "},
    {"u4   "},
    {"uo   "},
    {"uo6  "},
    {"uo3  "},
    {"uo4  "},
    {"ul   "},
    {"ul3  "},
    {"ul4  "},
    {"u0   "},
    {"u03  "},
    {"u04  "},
    {"up   "},
    {"up4  "},
    {"u/   "},
    {"u/3  "},
    {"u/4  "},
    {"j    "},
    {"j3   "},
    {"j4   "}
};

// ㄆ
static const uint8_t JY_index_p[][6] = {
    {"8    "},
    {"86   "},
    {"84   "},
    {"i    "},
    {"i6   "},
    {"i3   "},
    {"i4   "},
    {"9    "},
    {"96   "},
    {"93   "},
    {"94   "},
    {"o    "},
    {"o6   "},
    {"o4   "},
    {"l    "},
    {"l6   "},
    {"l3   "},
    {"l4   "},
    {".    "},
    {".6   "},
    {".3   "},
    {"0    "},
    {"06   "},
    {"04   "},
    {"p    "},
    {"p6   "},
    {"p4   "},
    {";    "},
    {";6   "},
    {";3   "},
    {";4   "},
    {"/    "},
    {"/6   "},
    {"/3   "},
    {"/4   "},
    {"u    "},
    {"u6   "},
    {"u3   "},
    {"u4   "},
    {"uo   "},
    {"uo3  "},
    {"ul   "},
    {"ul6  "},
    {"ul3  "},
    {"ul4  "},
    {"u0   "},
    {"u06  "},
    {"u03  "},
    {"u04  "},
    {"up   "},
    {"up6  "},
    {"up3  "},
    {"up4  "},
    {"u/   "},
    {"u/6  "},
    {"j    "},
    {"j6   "},
    {"j3   "},
    {"j4   "}
};

// ㄇ
static const uint8_t JY_index_m[][6] = {
    {"8    "},
    {"86   "},
    {"83   "},
    {"84   "},
    {"87   "},
    {"i    "},
    {"i6   "},
    {"i3   "},
    {"i4   "},
    {"i7   "},
    {"k7   "},
    {"96   "},
    {"93   "},
    {"94   "},
    {"o6   "},
    {"o3   "},
    {"o4   "},
    {"l    "},
    {"l6   "},
    {"l3   "},
    {"l4   "},
    {".6   "},
    {".3   "},
    {"0    "},
    {"06   "},
    {"03   "},
    {"04   "},
    {"p    "},
    {"p6   "},
    {"p4   "},
    {"p7   "},
    {";6   "},
    {";3   "},
    {"/    "},
    {"/6   "},
    {"/3   "},
    {"/4   "},
    {"u    "},
    {"u6   "},
    {"u3   "},
    {"u4   "},
    {"uo   "},
    {"uo4  "},
    {"ul   "},
    {"ul6  "},
    {"ul3  "},
    {"ul4  "},
    {"u.4  "},
    {"u06  "},
    {"u03  "},
    {"u04  "},
    {"up6  "},
    {"up3  "},
    {"u/6  "},
    {"u/4  "},
    {"j3   "},
    {"j4   "}
};

// ㄈ
static const uint8_t JY_index_f[][6] = {
    {"8    "},
    {"86   "},
    {"83   "},
    {"84   "},
    {"i6   "},
    {"o    "},
    {"o6   "},
    {"o3   "},
    {"o4   "},
    {".6   "},
    {".3   "},
    {"0    "},
    {"06   "},
    {"03   "},
    {"04   "},
    {"p    "},
    {"p6   "},
    {"p3   "},
    {"p4   "},
    {";    "},
    {";6   "},
    {";3   "},
    {";4   "},
    {"/    "},
    {"/6   "},
    {"/3   "},
    {"/4   "},
    {"j    "},
    {"j6   "},
    {"j3   "},
    {"j4   "}
};

// ㄉ
static const uint8_t JY_index_d[][6] = {
    {"8    "},
    {"86   "},
    {"83   "},
    {"84   "},
    {"k6   "},
    {"k7   "},
    {"9    "},
    {"93   "},
    {"94   "},
    {"o3   "},
    {"l    "},
    {"l3   "},
    {"l4   "},
    {".    "},
    {".3   "},
    {".4   "},
    {"0    "},
    {"03   "},
    {"04   "},
    {";    "},
    {";3   "},
    {";4   "},
    {"/    "},
    {"/3   "},
    {"/4   "},
    {"u    "},
    {"u6   "},
    {"u3   "},
    {"u4   "},
    {"uo   "},
    {"uo6  "},
    {"ul3  "},
    {"ul4  "},
    {"u.   "},
    {"u0   "},
    {"u03  "},
    {"u04  "},
    {"u/   "},
    {"u/3  "},
    {"u/4  "},
    {"j    "},
    {"j6   "},
    {"j3   "},
    {"j4   "},
    {"ji   "},
    {"ji6  "},
    {"ji3  "},
    {"ji4  "},
    {"jo   "},
    {"jo4  "},
    {"j0   "},
    {"j03  "},
    {"j04  "},
    {"jp   "},
    {"jp3  "},
    {"jp4  "},
    {"j/   "},
    {"j/3  "},
    {"j/4  "}
};

// ㄊ
static const uint8_t JY_index_t[][6] = {
    {"8    "},
    {"83   "},
    {"84   "},
    {"k6   "},
    {"k4   "},
    {"9    "},
    {"96   "},
    {"94   "},
    {"l    "},
    {"l6   "},
    {"l3   "},
    {"l4   "},
    {".    "},
    {".6   "},
    {".3   "},
    {".4   "},
    {"0    "},
    {"06   "},
    {"03   "},
    {"04   "},
    {";    "},
    {";6   "},
    {";3   "},
    {";4   "},
    {"/6   "},
    {"u    "},
    {"u6   "},
    {"u3   "},
    {"u4   "},
    {"uo   "},
    {"uo3  "},
    {"uo4  "},
    {"ul   "},
    {"ul6  "},
    {"ul3  "},
    {"ul4  "},
    {"u0   "},
    {"u06  "},
    {"u03  "},
    {"u04  "},
    {"u/   "},
    {"u/6  "},
    {"u/3  "},
    {"u/4  "},
    {"j    "},
    {"j6   "},
    {"j3   "},
    {"j4   "},
    {"j06  "},
    {"j03  "},
    {"j04  "},
    {"ji   "},
    {"ji6  "},
    {"ji3  "},
    {"ji4  "},
    {"jo   "},
    {"jo6  "},
    {"jo3  "},
    {"jo4  "},
    {"jp   "},
    {"jp2  "},
    {"jp4  "},
    {"j/   "},
    {"j/6  "},
    {"j/3  "},
    {"j/4  "}
};

// ㄋ
static const uint8_t JY_index_n[][6] = {
    {"86   "},
    {"83   "},
    {"84   "},
	{"87   "},
    {"k4   "},
    {"k7   "},
    {"93   "},
    {"94   "},
    {"o3   "},
    {"o4   "},
    {"l6   "},
    {"l3   "},
    {"l4   "},
    {".4   "},
    {"0    "},
    {"06   "},
    {"03   "},
    {"04   "},
    {"p4   "},
    {";6   "},
    {";3   "},
    {"/6   "},
    {"u6   "},
    {"u3   "},
    {"u4   "},
    {"uo   "},
    {"uo4  "},
    {"ul3  "},
    {"ul4  "},
    {"u.   "},
    {"u.6  "},
    {"u.3  "},
    {"u.4  "},
    {"u0   "},
    {"u06  "},
    {"u03  "},
    {"u04  "},
    {"up6  "},
    {"u;6  "},
    {"u;4  "},
    {"u/6  "},
    {"u/3  "},
    {"u/4  "},
    {"j6   "},
    {"j3   "},
    {"j4   "},
    {"ji6  "},
    {"ji4  "},
    {"j03  "},
    {"j/6  "},
    {"j/4  "},
    {"m3   "},
    {"mo4  "}
};

//ㄌ
static const uint8_t JY_index_l[][6] = {
    {"8    "},
    {"86   "},
    {"83   "},
    {"84   "},
    {"87   "},
    {"k4   "},
    {"k7   "},
    {"96   "},
    {"94   "},
    {"o    "},
    {"o6   "},
    {"o3   "},
    {"o4   "},
    {"l    "},
    {"l6   "},
    {"l3   "},
    {"l4   "},
    {"06   "},
    {"03   "},
    {"04   "},
    {";6   "},
    {";3   "},
    {";4   "},
    {"/6   "},
    {"/3   "},
    {"/4   "},
    {"u    "},
    {"u6   "},
    {"u3   "},
    {"u4   "},
    {"u7   "},
    {"uo   "},
    {"uo3  "},
    {"uo4  "},
    {"ul   "},
    {"ul6  "},
    {"ul3  "},
    {"ul4  "},
    {"u.   "},
    {"u.6  "},
    {"u.3  "},
    {"u.4  "},
    {"u06  "},
    {"u03  "},
    {"u04  "},
    {"up6  "},
    {"up3  "},
    {"up4  "},
    {"u;6  "},
    {"u;3  "},
    {"u;4  "},
    {"u/6  "},
    {"u/3  "},
    {"u/4  "},
    {".    "},
    {".6   "},
    {".3   "},
    {".4   "},
    {"j    "},
    {"j6   "},
    {"j3   "},
    {"j4   "},
    {"ji   "},
    {"ji6  "},
    {"ji3  "},
    {"ji4  "},
    {"j06  "},
    {"j03  "},
    {"j04  "},
    {"jp   "},
    {"jp6  "},
    {"jp4  "},
    {"j/   "},
    {"j/6  "},
    {"j/3  "},
    {"j/4  "},
    {"m6   "},
    {"m3   "},
    {"m4   "},
    {"mo4  "}
};

//ㄍ
static const uint8_t JY_index_g[][6] = {
    {"86   "},
    {"83   "},
    {"84   "},
    {"k    "},
    {"k6   "},
    {"k3   "},
    {"k4   "},
    {"9    "},
    {"96   "},
    {"94   "},
    {"o3   "},
    {"l    "},
    {"l3   "},
    {"l4   "},
    {".    "},
    {".3   "},
    {".4   "},
    {"0    "},
    {"03   "},
    {"04   "},
    {"p    "},
    {"p4   "},
    {";    "},
    {";3   "},
    {";4   "},
    {"/    "},
    {"/3   "},
    {"/4   "},
    {"j    "},
    {"j3   "},
    {"j4   "},
    {"j8   "},
    {"j83  "},
    {"j84  "},
    {"ji   "},
    {"ji6  "},
    {"ji3  "},
    {"ji4  "},
    {"j9   "},
    {"j93  "},
    {"j94  "},
    {"jo   "},
    {"jo3  "},
    {"jo4  "},
    {"j0   "},
    {"j03  "},
    {"j04  "},
    {"jp3  "},
    {"jp4  "},
    {"j;   "},
    {"j;3  "},
    {"j;4  "},
    {"j/   "},
    {"j/3  "},
    {"j/4  "}
};

//ㄎ
static const uint8_t JY_index_k[][6] = {
    {"8    "},
    {"83   "},
    {"k    "},
    {"k6   "},
    {"k3   "},
    {"k4   "},
    {"9    "},
    {"93   "},
    {"94   "},
    {"l    "},
    {"l3   "},
    {"l4   "},
    {".    "},
    {".3   "},
    {".4   "},
    {"0    "},
    {"03   "},
    {"04   "},
    {"p3   "},
    {"p4   "},
    {";    "},
    {";6   "},
    {";4   "},
    {"/    "},
    {"j    "},
    {"j3   "},
    {"j4   "},
    {"j8   "},
    {"j83  "},
    {"j84  "},
    {"ji4  "},
    {"j9   "},
    {"j93  "},
    {"j94  "},
    {"jo   "},
    {"jo6  "},
    {"jo3  "},
    {"jo4  "},
    {"j0   "},
    {"j03  "},
    {"jp   "},
    {"jp3  "},
    {"jp4  "},
    {"j;   "},
    {"j;6  "},
    {"j;4  "},
    {"j/   "},
    {"j/3  "},
    {"j/4  "}
};

//ㄏ
static const uint8_t JY_index_h[][6] = {
    {"8    "},
    {"86   "},
    {"84   "},
    {"k    "},
    {"k6   "},
    {"k4   "},
    {"9    "},
    {"96   "},
    {"93   "},
    {"94   "},
    {"o    "},
    {"l    "},
    {"l6   "},
    {"l3   "},
    {"l4   "},
    {".    "},
    {".6   "},
    {".3   "},
    {".4   "},
    {"0    "},
    {"06   "},
    {"03   "},
    {"04   "},
    {"p6   "},
    {"p3   "},
    {"p4   "},
    {";    "},
    {";6   "},
    {"/    "},
    {"/6   "},
    {"/4   "},
    {"j    "},
    {"j6   "},
    {"j3   "},
    {"j4   "},
    {"j8   "},
    {"j86  "},
    {"j84  "},
    {"ji   "},
    {"ji6  "},
    {"ji3  "},
    {"ji4  "},
    {"j96  "},
    {"j94  "},
    {"jo   "},
    {"jo6  "},
    {"jo3  "},
    {"jo4  "},
    {"j0   "},
    {"j06  "},
    {"j03  "},
    {"j04  "},
    {"jp   "},
    {"jp6  "},
    {"jp4  "},
    {"j;   "},
    {"j;6  "},
    {"j;3  "},
    {"j;4  "},
    {"j/   "},
    {"j/6  "},
    {"j/3  "},
    {"j/4  "}
};

//ㄐ
static const uint8_t JY_index_j[][6] = {
    {"u    "},
    {"u6   "},
    {"u3   "},
    {"u4   "},
    {"u8   "},
    {"u86  "},
    {"u83  "},
    {"u84  "},
    {"uo   "},
    {"uo6  "},
    {"uo3  "},
    {"uo4  "},
    {"ul   "},
    {"ul6  "},
    {"ul3  "},
    {"ul4  "},
    {"u.   "},
    {"u.3  "},
    {"u.4  "},
    {"u0   "},
    {"u03  "},
    {"u04  "},
    {"up   "},
    {"up3  "},
    {"up4  "},
    {"u/   "},
    {"u/3  "},
    {"u/4  "},
    {"u;   "},
    {"u;3  "},
    {"u;4  "},
    {"m    "},
    {"m6   "},
    {"m3   "},
    {"m4   "},
    {"mo   "},
    {"mo6  "},
    {"m0   "},
    {"m03  "},
    {"m04  "},
    {"mp   "},
    {"mp4  "},
    {"m/   "},
    {"m/3  "}
};

//ㄑ
static const uint8_t JY_index_q[][6] = {
    {"u    "},
    {"u6   "},
    {"u3   "},
    {"u4   "},
    {"u8   "},
    {"u83  "},
    {"u84  "},
    {"uo   "},
    {"uo6  "},
    {"uo3  "},
    {"uo4  "},
    {"ul   "},
    {"ul6  "},
    {"ul3  "},
    {"ul4  "},
    {"u.   "},
    {"u.6  "},
    {"u.3  "},
    {"u0   "},
    {"u06  "},
    {"u03  "},
    {"u04  "},
    {"up   "},
    {"up6  "},
    {"up3  "},
    {"up4  "},
    {"u;   "},
    {"u;6  "},
    {"u;3  "},
    {"u;4  "},
    {"u/   "},
    {"u/6  "},
    {"u/3  "},
    {"u/4  "},
    {"m    "},
    {"m6   "},
    {"m3   "},
    {"m4   "},
    {"mo   "},
    {"mo6  "},
    {"mo4  "},
    {"m0   "},
    {"m06  "},
    {"m03  "},
    {"m04  "},
    {"mp   "},
    {"mp6  "},
    {"m/   "},
    {"m/6  "}
};

//ㄒ
static const uint8_t JY_index_x[][6] = {
    {"u    "},
    {"u6   "},
    {"u3   "},
    {"u4   "},
    {"u8   "},
    {"u86  "},
    {"u84  "},
    {"uo   "},
    {"uo6  "},
    {"uo3  "},
    {"uo4  "},
    {"ul   "},
    {"ul3  "},
    {"ul4  "},
    {"u.   "},
    {"u.3  "},
    {"u.4  "},
    {"u0   "},
    {"u06  "},
    {"u03  "},
    {"u04  "},
    {"up   "},
    {"up6  "},
    {"up3  "},
    {"up4  "},
    {"u;   "},
    {"u;6  "},
    {"u;3  "},
    {"u;4  "},
    {"u/   "},
    {"u/6  "},
    {"u/3  "},
    {"u/4  "},
    {"m    "},
    {"m6   "},
    {"m3   "},
    {"m4   "},
    {"mo   "},
    {"mo6  "},
    {"mo3  "},
    {"mo4  "},
    {"m0   "},
    {"m06  "},
    {"m03  "},
    {"m04  "},
    {"mp   "},
    {"mp6  "},
    {"mp4  "},
    {"m/   "},
    {"m/6  "},
    {"m/4  "}
};

//ㄓ
static const uint8_t JY_index_zh[][6] = {
    {"     "},
    {"6    "},
    {"3    "},
    {"4    "},
    {"8    "},
    {"86   "},
    {"83   "},
    {"84   "},
    {"k    "},
    {"k6   "},
    {"k3   "},
    {"k4   "},
    {"9    "},
    {"96   "},
    {"93   "},
    {"94   "},
    {"l    "},
    {"l6   "},
    {"l3   "},
    {"l4   "},
    {".    "},
    {".6   "},
    {".3   "},
    {".4   "},
    {"0    "},
    {"03   "},
    {"04   "},
    {"p    "},
    {"p3   "},
    {"p4   "},
    {";    "},
    {";3   "},
    {";4   "},
    {"/    "},
    {"/3   "},
    {"/4   "},
    {"j    "},
    {"j6   "},
    {"j3   "},
    {"j4   "},
    {"j8   "},
    {"j83  "},
    {"ji   "},
    {"ji6  "},
    {"j9   "},
    {"j93  "},
    {"j94  "},
    {"j;   "},
    {"j;4  "},
    {"jo   "},
    {"jo4  "},
    {"j0   "},
    {"j03  "},
    {"j04  "},
    {"jp   "},
    {"jp3  "},
    {"j/   "},
    {"j/3  "},
    {"j/4  "}
};

//ㄔ
static const uint8_t JY_index_ch[][6] = {
    {"     "},
    {"6    "},
    {"3    "},
    {"4    "},
    {"8    "},
    {"86   "},
    {"83   "},
    {"84   "},
    {"k    "},
    {"k3   "},
    {"k4   "},
    {"9    "},
    {"96   "},
    {"94   "},
    {"l    "},
    {"l6   "},
    {"l3   "},
    {".    "},
    {".6   "},
    {".3   "},
    {".4   "},
    {"0    "},
    {"06   "},
    {"03   "},
    {"04   "},
    {"p    "},
    {"p6   "},
    {"p4   "},
    {";    "},
    {";6   "},
    {";3   "},
    {";4   "},
    {"/    "},
    {"/6   "},
    {"/3   "},
    {"/4   "},
    {"j    "},
    {"j6   "},
    {"j3   "},
    {"j4   "},
    {"ji   "},
    {"ji4  "},
    {"j93  "},
    {"j94  "},
    {"jo   "},
    {"jo6  "},
    {"j0   "},
    {"j06  "},
    {"j03  "},
    {"j04  "},
    {"j;   "},
    {"j;6  "},
    {"j;3  "},
    {"j;4  "},
    {"jp   "},
    {"jp6  "},
    {"jp3  "},
    {"j/   "},
    {"j/6  "},
    {"j/3  "},
    {"j/4  "}
};

//ㄕ
static const uint8_t JY_index_sh[][6] = {
    {"     "},
    {"6    "},
    {"3    "},
    {"4    "},
    {"8    "},
    {"86   "},
    {"83   "},
    {"84   "},
    {"k    "},
    {"k6   "},
    {"k3   "},
    {"k4   "},
    {"9    "},
    {"93   "},
    {"94   "},
    {"o6   "},
    {"l    "},
    {"l6   "},
    {"l3   "},
    {"l4   "},
    {".    "},
    {".3   "},
    {".4   "},
    {"0    "},
    {"03   "},
    {"04   "},
    {"p    "},
    {"p6   "},
    {"p3   "},
    {"p4   "},
    {";    "},
    {";3   "},
    {";4   "},
    {"/    "},
    {"/6   "},
    {"/3   "},
    {"/4   "},
    {"j    "},
    {"j6   "},
    {"j3   "},
    {"j4   "},
    {"j8   "},
    {"j83  "},
    {"j84  "},
    {"ji   "},
    {"ji4  "},
    {"j9   "},
    {"j93  "},
    {"j94  "},
    {"jo2  "},
    {"jo3  "},
    {"jo4  "},
    {"j0   "},
    {"j04  "},
    {"jp3  "},
    {"jp4  "},
    {"j;   "},
    {"j;3  "}
};

//ㄖ
static const uint8_t JY_index_r[][6] = {
    {"4    "},
    {"k3   "},
    {"k4   "},
    {"l6   "},
    {"l3   "},
    {"l4   "},
    {".6   "},
    {".4   "},
    {"06   "},
    {"03   "},
    {"p6   "},
    {"p3   "},
    {"p4   "},
    {";6   "},
    {";3   "},
    {";4   "},
    {"/    "},
    {"/6   "},
    {"j6   "},
    {"j3   "},
    {"j4   "},
    {"ji4  "},
    {"jo6  "},
    {"jo3  "},
    {"jo4  "},
    {"j06  "},
    {"j03  "},
    {"jp6  "},
    {"jp4  "},
    {"j/6  "},
    {"j/3  "}
};

//ㄗ
static const uint8_t JY_index_z[][6] = {
    {"     "},
    {"6    "},
    {"3    "},
    {"4    "},
    {"8    "},
    {"86   "},
    {"k6   "},
    {"k3   "},
    {"k4   "},
    {"9    "},
    {"93   "},
    {"94   "},
    {"l    "},
    {"l6   "},
    {"l3   "},
    {"l4   "},
    {"o6   "},
    {".    "},
    {".3   "},
    {".4   "},
    {"0    "},
    {"06   "},
    {"03   "},
    {"04   "},
    {"p3   "},
    {"p4   "},
    {";    "},
    {";3   "},
    {";4   "},
    {"/    "},
    {"/4   "},
    {"j    "},
    {"j6   "},
    {"j3   "},
    {"ji   "},
    {"ji6  "},
    {"ji3  "},
    {"ji4  "},
    {"jo3  "},
    {"jo4  "},
    {"jp   "},
    {"jp3  "},
    {"j0   "},
    {"j03  "},
    {"j04  "},
    {"j/   "},
    {"j/3  "},
    {"j/4  "}
};

//ㄘ
static const uint8_t JY_index_c[][6] = {
    {"     "},
    {"6    "},
    {"3    "},
    {"4    "},
    {"8    "},
    {"k4   "},
    {"9    "},
    {"96   "},
    {"93   "},
    {"94   "},
    {"l    "},
    {"l6   "},
    {"l3   "},
    {".4   "},
    {"0    "},
    {"06   "},
    {"03   "},
    {"04   "},
    {"p    "},
    {"p6   "},
    {";    "},
    {";6   "},
    {"/    "},
    {"/6   "},
    {"/4   "},
    {"j    "},
    {"j6   "},
    {"j4   "},
    {"ji   "},
    {"ji6  "},
    {"ji3  "},
    {"ji4  "},
    {"jo   "},
    {"jo3  "},
    {"jo4  "},
    {"j0   "},
    {"j06  "},
    {"j04  "},
    {"jp   "},
    {"jp6  "},
    {"jp3  "},
    {"jp4  "},
    {"j/   "},
    {"j/6  "}
};

//ㄙ
static const uint8_t JY_index_s[][6] = {
    {"     "},
    {"3    "},
    {"4    "},
    {"8    "},
    {"83   "},
    {"84   "},
    {"k4   "},
    {"9    "},
    {"94   "},
    {"0    "},
    {"03   "},
    {"04   "},
    {"l    "},
    {"l3   "},
    {"l4   "},
    {".    "},
    {".3   "},
    {".4   "},
    {"p    "},
    {";    "},
    {";3   "},
    {";4   "},
    {"/    "},
    {"j    "},
    {"j6   "},
    {"j4   "},
    {"ji   "},
    {"ji3  "},
    {"jo   "},
    {"jo6  "},
    {"jo3  "},
    {"jo4  "},
    {"j0   "},
    {"j04  "},
    {"jp   "},
    {"jp3  "},
    {"j/   "},
    {"j/3  "},
    {"j/4  "}
};

static const uint8_t JY_index_a[][6] = {
    {"     "},
    {"6    "},
    {"3    "},
    {"4    "},
    {"7    "}
};

static const uint8_t JY_index_o[][6] = {
    {"     "},
    {"6    "},
    {"3    "}
};

static const uint8_t JY_index_e[][6] = {
    {"     "},
    {"6    "},
    {"3    "},
    {"4    "}
};

static const uint8_t JY_index_ei[][6] = {
    {"4    "}
};

static const uint8_t JY_index_ai[][6] = {
    {"     "},
    {"6    "},
    {"3    "},
    {"4    "}
};

static const uint8_t JY_index_ao[][6] = {
    {"     "},
    {"6    "},
    {"3    "},
    {"4    "}
};

static const uint8_t JY_index_ou[][6] = {
    {"     "},
    {"3    "},
    {"4    "}
};

static const uint8_t JY_index_an[][6] = {
    {"     "},
    {"3    "},
    {"4    "}
};

static const uint8_t JY_index_en[][6] = {
    {"     "},
    {"4    "}
};

static const uint8_t JY_index_ang[][6] = {
    {"     "},
    {"6    "},
    {"4    "}
};

static const uint8_t JY_index_er[][6] = {
    {"6    "},
    {"3    "},
    {"4    "},
    {"7    "}
};

// ㄧ
static const uint8_t JY_index_y[][6] = {
    {"     "},
    {"6    "},
    {"3    "},
    {"4    "},
    {"8    "},
    {"86   "},
    {"83   "},
    {"84   "},
    {"87   "},
    {"i    "},
    {"i7   "},
    {"o    "},
    {"o6   "},
    {"o3   "},
    {"o4   "},
    {"96   "},
    {"l    "},
    {"l6   "},
    {"l3   "},
    {"l4   "},
    {"l7   "},
    {".    "},
    {".6   "},
    {".3   "},
    {".4   "},
    {"0    "},
    {"06   "},
    {"03   "},
    {"04   "},
    {"p    "},
    {"p6   "},
    {"p3   "},
    {"p4   "},
    {";    "},
    {";6   "},
    {";3   "},
    {";4   "},
    {"/    "},
    {"/6   "},
    {"/3   "},
    {"/4   "}
};

// ㄨ
static const uint8_t JY_index_w[][6] = {
    {"     "},
    {"6    "},
    {"3    "},
    {"4    "},
    {"8    "},
    {"86   "},
    {"83   "},
    {"84   "},
    {"i    "},
    {"i3   "},
    {"i4   "},
    {"9    "},
    {"94   "},
    {"o    "},
    {"o6   "},
    {"o3   "},
    {"o4   "},
    {"0    "},
    {"06   "},
    {"03   "},
    {"04   "},
    {"p    "},
    {"p6   "},
    {"p3   "},
    {"p4   "},
    {";    "},
    {";6   "},
    {";3   "},
    {";4   "},
    {"/    "},
    {"/3   "},
    {"/4   "}
};

// ㄩ
static const uint8_t JY_index_yu[][6] = {
    {"     "},
    {"6    "},
    {"3    "},
    {"4    "},
    {"o    "},
    {"o4   "},
    {"0    "},
    {"06   "},
    {"03   "},
    {"04   "},
    {"p    "},
    {"p6   "},
    {"p3   "},
    {"p4   "},
    {"/    "},
    {"/6   "},
    {"/3   "},
    {"/4   "}
};

static const uint8_t JY_index_end[][6] = {"     "};

/* 注音輸入法處理函式
 * 參數:
 *   input_jy_val: 輸入的注音字串
 *   get_hanzi: 輸出的漢字緩衝區
 *   hh: 輸出漢字的個數指標
 * 回傳值:
 *   true: 成功找到對應的漢字
 *   false: 未找到對應的漢字
 */
bool jy_ime(uint8_t *input_jy_val, uint8_t *get_hanzi, uint16_t *hh)
{
    uint8_t         jy_ime_temp[8];        // 暫存注音比對用緩衝區
    uint8_t         jy_ime_temp1[8];       // 暫存輸入注音緩衝區
    uint8_t         jy_ime_cmp;            // 儲存第一個注音符號
    uint8_t         jy_i;                  // 迴圈計數器
    uint16_t        g_hanzi_num;           // 找到的漢字數量
    const uint8_t * add = NULL;            // 指向漢字表的指標

    // 複製輸入的注音到暫存區
    memcpy(jy_ime_temp1, input_jy_val, 6);
    // 取得第一個注音符號
    jy_ime_cmp = input_jy_val[0];

    // 輸出除錯訊息
    (void)printf("jy_ime_cmp: %c\n", jy_ime_cmp);

    // 處理ㄅ(1)開頭的注音
    if (jy_ime_cmp == '1')
    {
        // 遍歷所有ㄅ的組合可能
        for (jy_i = 0; jy_i < sizeofB; jy_i++)
        {
            // 複製注音索引表的內容到暫存區
            memcpy(jy_ime_temp, JY_index_b[jy_i], 5);
            // 比對輸入的注音是否匹配
            if (memcmp(&jy_ime_temp1[1], jy_ime_temp, 5) == 0)
            {
                // 根據索引選擇對應的漢字表
                switch (jy_i)
                {
                case 0: add  = JY_mb_ba; break;
                case 1: add  = JY_mb_ba2; break;
                case 2: add  = JY_mb_ba3; break;
                case 3: add  = JY_mb_ba4; break;
                case 4: add  = JY_mb_ba5; break;
                case 5: add  = JY_mb_bo; break;
                case 6: add  = JY_mb_bo2; break;
                case 7: add  = JY_mb_bo3; break;
                case 8: add  = JY_mb_bo4; break;
                case 9: add  = JY_mb_bo5; break;
                case 10: add  = JY_mb_bai; break;
                case 11: add  = JY_mb_bai2; break;
                case 12: add  = JY_mb_bai3; break;
                case 13: add  = JY_mb_bai4; break;
                case 14: add  = JY_mb_bei; break;
                case 15: add  = JY_mb_bei3; break;
                case 16: add  = JY_mb_bei4; break;
                case 17: add  = JY_mb_bao; break;
                case 18: add  = JY_mb_bao2; break;
                case 19: add  = JY_mb_bao3; break;
                case 20: add  = JY_mb_bao4; break;
                case 21: add  = JY_mb_ban; break;
                case 22: add  = JY_mb_ban3; break;
                case 23: add  = JY_mb_ban4; break;
                case 24: add  = JY_mb_ben; break;
                case 25: add  = JY_mb_ben3; break;
                case 26: add  = JY_mb_ben4; break;
                case 27: add  = JY_mb_bang; break;
                case 28: add  = JY_mb_bang3; break;
                case 29: add  = JY_mb_bang4; break;
                case 30: add  = JY_mb_beng; break;
                case 31: add  = JY_mb_beng2; break;
                case 32: add  = JY_mb_beng3; break;
                case 33: add  = JY_mb_beng4; break;
                case 34: add  = JY_mb_bi; break;
                case 35: add  = JY_mb_bi2; break;
                case 36: add  = JY_mb_bi3; break;
                case 37: add  = JY_mb_bi4; break;
                case 38: add  = JY_mb_bie; break;
                case 39: add  = JY_mb_bie2; break;
                case 40: add  = JY_mb_bie3; break;
                case 41: add  = JY_mb_bie4; break;
                case 42: add  = JY_mb_biao; break;
                case 43: add  = JY_mb_biao3; break;
                case 44: add  = JY_mb_biao4; break;
                case 45: add  = JY_mb_bian; break;
                case 46: add  = JY_mb_bian3; break;
                case 47: add  = JY_mb_bian4; break;
                case 48: add  = JY_mb_bin; break;
                case 49: add  = JY_mb_bin4; break;
                case 50: add  = JY_mb_bing; break;
                case 51: add  = JY_mb_bing3; break;
                case 52: add  = JY_mb_bing4; break;
                case 53: add  = JY_mb_bu; break;
                case 54: add  = JY_mb_bu3; break;
                case 55: add  = JY_mb_bu4; break;
                }
                if (add != NULL)
                {
                    g_hanzi_num = (uint16_t)(strlen((char *)add)); //注音ㄅ的个数
                    memcpy(get_hanzi, add, g_hanzi_num);         //把找到的汉字放在缓冲区
                    *hh         = g_hanzi_num / 2;
                    return true;
                }
                // 如果 add 為 NULL (例如 default case)，則繼續迴圈查找下一個韻母
            }
        }
        return false;   // 'ㄅ' 迴圈結束，未找到匹配韻母
    }

    //首字母为 'ㄆ' (q)
    if (jy_ime_cmp == 'q')
    {
        for (jy_i = 0; jy_i < sizeofP; jy_i++)                     // 16次
        {
            memcpy(jy_ime_temp, JY_index_p[jy_i], 5);              //   ai,an,ang,ao，ei,...   16个
            if (memcmp(&jy_ime_temp1[1], jy_ime_temp, 5) == 0)     // 输入的拼音和'a'的第2个韵母比较
            {
                switch (jy_i)
                {
                case 0: add  = JY_mb_pa; break;
                case 1: add  = JY_mb_pa2; break;
                case 2: add  = JY_mb_pa4; break;
                case 3: add  = JY_mb_po; break;
                case 4: add  = JY_mb_po2; break;
                case 5: add  = JY_mb_po3; break;
                case 6: add  = JY_mb_po4; break;
                case 7: add  = JY_mb_pai; break;
                case 8: add  = JY_mb_pai2; break;
                case 9: add  = JY_mb_pai3; break;
                case 10: add  = JY_mb_pai4; break;
                case 11: add  = JY_mb_pei; break;
                case 12: add  = JY_mb_pei2; break;
                case 13: add  = JY_mb_pei4; break;
                case 14: add  = JY_mb_pao; break;
                case 15: add  = JY_mb_pao2; break;
                case 16: add  = JY_mb_pao3; break;
                case 17: add  = JY_mb_pao4; break;
                case 18: add  = JY_mb_pou; break;
                case 19: add  = JY_mb_pou2; break;
                case 20: add  = JY_mb_pou3; break;
                case 21: add  = JY_mb_pan; break;
                case 22: add  = JY_mb_pan2; break;
                case 23: add  = JY_mb_pan4; break;
                case 24: add  = JY_mb_pen; break;
                case 25: add  = JY_mb_pen2; break;
                case 26: add  = JY_mb_pen4; break;
                case 27: add  = JY_mb_pang; break;
                case 28: add  = JY_mb_pang2; break;
                case 29: add  = JY_mb_pang3; break;
                case 30: add  = JY_mb_pang4; break;
                case 31: add  = JY_mb_peng; break;
                case 32: add  = JY_mb_peng2; break;
                case 33: add  = JY_mb_peng3; break;
                case 34: add  = JY_mb_peng4; break;
                case 35: add  = JY_mb_pi; break;
                case 36: add  = JY_mb_pi2; break;
                case 37: add  = JY_mb_pi3; break;
                case 38: add  = JY_mb_pi4; break;
                case 39: add  = JY_mb_pie; break;
                case 40: add  = JY_mb_pie3; break;
                case 41: add  = JY_mb_piao; break;
                case 42: add  = JY_mb_piao2; break;
                case 43: add  = JY_mb_piao3; break;
                case 44: add  = JY_mb_piao4; break;
                case 45: add  = JY_mb_pian; break;
                case 46: add  = JY_mb_pian2; break;
                case 47: add  = JY_mb_pian3; break;
                case 48: add  = JY_mb_pian4; break;
                case 49: add  = JY_mb_pin; break;
                case 50: add  = JY_mb_pin2; break;
                case 51: add  = JY_mb_pin3; break;
                case 52: add  = JY_mb_pin4; break;
                case 53: add  = JY_mb_ping; break;
                case 54: add  = JY_mb_ping2; break;
                case 55: add  = JY_mb_pu; break;
                case 56: add  = JY_mb_pu2; break;
                case 57: add  = JY_mb_pu3; break;
                case 58: add  = JY_mb_pu4; break;
                }
                if (add != NULL)
                {
                    g_hanzi_num = (uint16_t)strlen((char *)add); //   注音ㄆ的个数
                    memcpy(get_hanzi, add, g_hanzi_num);       //  把找到的汉字放在缓冲区
                    *hh         = g_hanzi_num / 2;
                    return true;
                }
            }
        }
        return false;
    }

    //首字母为 'ㄇ'
    if (jy_ime_cmp == 'a')
    {
        for (jy_i = 0; jy_i < sizeofM; jy_i++)                     // 16次
        {
            memcpy(jy_ime_temp, JY_index_m[jy_i], 5);              //   ai,an,ang,ao，ei,...   16个
            if (memcmp(&jy_ime_temp1[1], jy_ime_temp, 5) == 0)     // 输入的拼音和'a'的第2个韵母比较
            {
                switch (jy_i)
                {
                case 0: add  = JY_mb_ma; break;
                case 1: add  = JY_mb_ma2; break;
                case 2: add  = JY_mb_ma3; break;
                case 3: add  = JY_mb_ma4; break;
                case 4: add  = JY_mb_ma5; break;
                case 5: add  = JY_mb_mo; break;
                case 6: add  = JY_mb_mo2; break;
                case 7: add  = JY_mb_mo3; break;
                case 8: add  = JY_mb_mo4; break;
                case 9: add  = JY_mb_mo5; break;
                case 10: add  = JY_mb_me5; break;
                case 11: add  = JY_mb_mai2; break;
                case 12: add  = JY_mb_mai3; break;
                case 13: add  = JY_mb_mai4; break;
                case 14: add  = JY_mb_mei2; break;
                case 15: add  = JY_mb_mei3; break;
                case 16: add  = JY_mb_mei4; break;
                case 17: add  = JY_mb_mao; break;
                case 18: add  = JY_mb_mao2; break;
                case 19: add  = JY_mb_mao3; break;
                case 20: add  = JY_mb_mao4; break;
                case 21: add  = JY_mb_mou2; break;
                case 22: add  = JY_mb_mou3; break;
                case 23: add  = JY_mb_man; break;
                case 24: add  = JY_mb_man2; break;
                case 25: add  = JY_mb_man3; break;
                case 26: add  = JY_mb_man4; break;
                case 27: add  = JY_mb_men; break;
                case 28: add  = JY_mb_men2; break;
                case 29: add  = JY_mb_men4; break;
                case 30: add  = JY_mb_men5; break;
                case 31: add  = JY_mb_mang2; break;
                case 32: add  = JY_mb_mang3; break;
                case 33: add  = JY_mb_meng; break;
                case 34: add  = JY_mb_meng2; break;
                case 35: add  = JY_mb_meng3; break;
                case 36: add  = JY_mb_meng4; break;
                case 37: add  = JY_mb_mi; break;
                case 38: add  = JY_mb_mi2; break;
                case 39: add  = JY_mb_mi3; break;
                case 40: add  = JY_mb_mi4; break;
                case 41: add  = JY_mb_mie; break;
                case 42: add  = JY_mb_mie4; break;
                case 43: add  = JY_mb_miao; break;
                case 44: add  = JY_mb_miao2; break;
                case 45: add  = JY_mb_miao3; break;
                case 46: add  = JY_mb_miao4; break;
                case 47: add  = JY_mb_miu4; break;
                case 48: add  = JY_mb_mian2; break;
                case 49: add  = JY_mb_mian3; break;
                case 50: add  = JY_mb_mian4; break;
                case 51: add  = JY_mb_min2; break;
                case 52: add  = JY_mb_min3; break;
                case 53: add  = JY_mb_ming2; break;
                case 54: add  = JY_mb_ming4; break;
                case 55: add  = JY_mb_mu3; break;
                case 56: add  = JY_mb_mu4; break;
                }
                if (add != NULL)
                {
                    g_hanzi_num = (uint16_t)strlen((char *)add); //   拼音  a 的个数
                    memcpy(get_hanzi, add, g_hanzi_num);       //  把找到的汉字放在缓冲区
                    *hh         = g_hanzi_num / 2;
                    return true;
                }
            }
        }
        return false;
    }

    //首字母为 'ㄈ'
    if (jy_ime_cmp == 'z')
    {
        for (jy_i = 0; jy_i < sizeofF; jy_i++)                     // 16次
        {
            memcpy(jy_ime_temp, JY_index_f[jy_i], 5);              //   ai,an,ang,ao，ei,...   16个
            if (memcmp(&jy_ime_temp1[1], jy_ime_temp, 5) == 0)     // 输入的拼音和'a'的第2个韵母比较
            {
                (void)printf("jy_i==%d\n", jy_i);
                switch (jy_i)
                {
                case 0: add = JY_mb_fa; break;
                case 1: add = JY_mb_fa2; break;
                case 2: add = JY_mb_fa3; break;
                case 3: add = JY_mb_fa4; break;
                case 4: add = JY_mb_fo2; break;
                case 5: add = JY_mb_fei; break;
                case 6: add = JY_mb_fei2; break;
                case 7: add = JY_mb_fei3; break;
                case 8: add = JY_mb_fei4; break;
                case 9: add = JY_mb_fou2; break;
                case 10: add = JY_mb_fou3; break;
                case 11: add = JY_mb_fan; break;
                case 12: add = JY_mb_fan2; break;
                case 13: add = JY_mb_fan3; break;
                case 14: add = JY_mb_fan4; break;
                case 15: add = JY_mb_fen; break;
                case 16: add = JY_mb_fen2; break;
                case 17: add = JY_mb_fen3; break;
                case 18: add = JY_mb_fen4; break;
                case 19: add = JY_mb_fang; break;
                case 20: add = JY_mb_fang2; break;
                case 21: add = JY_mb_fang3; break;
                case 22: add = JY_mb_fang4; break;
                case 23: add = JY_mb_feng; break;
                case 24: add = JY_mb_feng2; break;
                case 25: add = JY_mb_feng3; break;
                case 26: add = JY_mb_feng4; break;
                case 27: add = JY_mb_fu; break;
                case 28: add = JY_mb_fu2; break;
                case 29: add = JY_mb_fu3; break;
                case 30: add = JY_mb_fu4; break;
                }
                if (add != NULL)
                {
                    g_hanzi_num = (uint16_t)strlen((char *)add); //   拼音  a 的个数
                    memcpy(get_hanzi, add, g_hanzi_num);       //  把找到的汉字放在缓冲区
                    *hh         = g_hanzi_num / 2;
                    return true;
                }
            }
        }
        return false;
    }

    //首字母为 'ㄉ' (2)
    if (jy_ime_cmp == '2')
    {
        for (jy_i = 0; jy_i < sizeofD; jy_i++)                     // 16次
        {
            memcpy(jy_ime_temp, JY_index_d[jy_i], 5);              //   ai,an,ang,ao，ei,...   16个
            if (memcmp(&jy_ime_temp1[1], jy_ime_temp, 5) == 0)     // 输入的拼音和'a'的第2个韵母比较
            {
                switch (jy_i)
                {
                case 0: add  = JY_mb_da; break;
                case 1: add  = JY_mb_da2; break;
                case 2: add  = JY_mb_da3; break;
                case 3: add  = JY_mb_da4; break;
                case 4: add  = JY_mb_de2; break;
                case 5: add  = JY_mb_de5; break;
                case 6: add  = JY_mb_dai; break;
                case 7: add  = JY_mb_dai3; break;
                case 8: add  = JY_mb_dai4; break;
                case 9: add  = JY_mb_dei3; break;
                case 10: add  = JY_mb_dao; break;
                case 11: add  = JY_mb_dao3; break;
                case 12: add  = JY_mb_dao4; break;
                case 13: add  = JY_mb_dou; break;
                case 14: add  = JY_mb_dou3; break;
                case 15: add  = JY_mb_dou4; break;
                case 16: add  = JY_mb_dan; break;
                case 17: add  = JY_mb_dan3; break;
                case 18: add  = JY_mb_dan4; break;
                case 19: add  = JY_mb_dang; break;
                case 20: add  = JY_mb_dang3; break;
                case 21: add  = JY_mb_dang4; break;
                case 22: add  = JY_mb_deng; break;
                case 23: add  = JY_mb_deng3; break;
                case 24: add  = JY_mb_deng4; break;
                case 25: add  = JY_mb_di; break;
                case 26: add  = JY_mb_di2; break;
                case 27: add  = JY_mb_di3; break;
                case 28: add  = JY_mb_di4; break;
                case 29: add  = JY_mb_die; break;
                case 30: add  = JY_mb_die2; break;
                case 31: add  = JY_mb_diao3; break;
                case 32: add  = JY_mb_diao4; break;
                case 33: add  = JY_mb_diu; break;
                case 34: add  = JY_mb_dian; break;
                case 35: add  = JY_mb_dian3; break;
                case 36: add  = JY_mb_dian4; break;
                case 37: add  = JY_mb_ding; break;
                case 38: add  = JY_mb_ding3; break;
                case 39: add  = JY_mb_ding4; break;
                case 40: add  = JY_mb_du; break;
                case 41: add  = JY_mb_du2; break;
                case 42: add  = JY_mb_du3; break;
                case 43: add  = JY_mb_du4; break;
                case 44: add  = JY_mb_duo; break;
                case 45: add  = JY_mb_duo2; break;
                case 46: add  = JY_mb_duo3; break;
                case 47: add  = JY_mb_duo4; break;
                case 48: add  = JY_mb_dui; break;
                case 49: add  = JY_mb_dui4; break;
                case 50: add  = JY_mb_duan; break;
                case 51: add  = JY_mb_duan3; break;
                case 52: add  = JY_mb_duan4; break;
                case 53: add  = JY_mb_dun; break;
                case 54: add  = JY_mb_dun3; break;
                case 55: add  = JY_mb_dun4; break;
                case 56: add  = JY_mb_dong; break;
                case 57: add  = JY_mb_dong3; break;
                case 58: add  = JY_mb_dong4; break;
                }
                if (add != NULL)
                {
                    g_hanzi_num = (uint16_t)(strlen((char *)add)); //拼音a的个数
                    memcpy(get_hanzi, add, g_hanzi_num);         //把找到的汉字放在缓冲区
                    *hh         = g_hanzi_num / 2;
                    return true;
                }
            }
        }
        return false;
    }

    //首字母为 'ㄊ' (w)
    if (jy_ime_cmp == 'w')
    {
        for (jy_i = 0; jy_i < sizeofT; jy_i++)                     // 16次
        {
            memcpy(jy_ime_temp, JY_index_t[jy_i], 5);              //   ai,an,ang,ao，ei,...   16个
            if (memcmp(&jy_ime_temp1[1], jy_ime_temp, 5) == 0)     // 输入的拼音和'a'的第2个韵母比较
            {
                switch (jy_i)
                {
                case 0: add  = JY_mb_ta; break;
                case 1: add  = JY_mb_ta3; break;
                case 2: add  = JY_mb_ta4; break;
                case 3: add  = JY_mb_te2; break;
                case 4: add  = JY_mb_te4; break;
                case 5: add  = JY_mb_tai; break;
                case 6: add  = JY_mb_tai2; break;
                case 7: add  = JY_mb_tai4; break;
                case 8: add  = JY_mb_tao; break;
                case 9: add  = JY_mb_tao2; break;
                case 10: add  = JY_mb_tao3; break;
                case 11: add  = JY_mb_tao4; break;
                case 12: add  = JY_mb_tou; break;
                case 13: add  = JY_mb_tou2; break;
                case 14: add  = JY_mb_tou3; break;
                case 15: add  = JY_mb_tou4; break;
                case 16: add  = JY_mb_tan; break;
                case 17: add  = JY_mb_tan2; break;
                case 18: add  = JY_mb_tan3; break;
                case 19: add  = JY_mb_tan4; break;
                case 20: add  = JY_mb_tang; break;
                case 21: add  = JY_mb_tang2; break;
                case 22: add  = JY_mb_tang3; break;
                case 23: add  = JY_mb_tang4; break;
                case 24: add  = JY_mb_teng2; break;
                case 25: add  = JY_mb_ti; break;
                case 26: add  = JY_mb_ti2; break;
                case 27: add  = JY_mb_ti3; break;
                case 28: add  = JY_mb_ti4; break;
                case 29: add  = JY_mb_tie; break;
                case 30: add  = JY_mb_tie3; break;
                case 31: add  = JY_mb_tie4; break;
                case 32: add  = JY_mb_tiao; break;
                case 33: add  = JY_mb_tiao2; break;
                case 34: add  = JY_mb_tiao3; break;
                case 35: add  = JY_mb_tiao4; break;
                case 36: add  = JY_mb_tian; break;
                case 37: add  = JY_mb_tian2; break;
                case 38: add  = JY_mb_tian3; break;
                case 39: add  = JY_mb_tian4; break;
                case 40: add  = JY_mb_ting; break;
                case 41: add  = JY_mb_ting2; break;
                case 42: add  = JY_mb_ting3; break;
                case 43: add  = JY_mb_ting4; break;
                case 44: add  = JY_mb_tu; break;
                case 45: add  = JY_mb_tu2; break;
                case 46: add  = JY_mb_tu3; break;
                case 47: add  = JY_mb_tu4; break;
                case 48: add  = JY_mb_tuan2; break;
                case 49: add  = JY_mb_tuan3; break;
                case 50: add  = JY_mb_tuan4; break;
                case 51: add  = JY_mb_tuo; break;
                case 52: add  = JY_mb_tuo2; break;
                case 53: add  = JY_mb_tuo3; break;
                case 54: add  = JY_mb_tuo4; break;
                case 55: add  = JY_mb_tui; break;
                case 56: add  = JY_mb_tui2; break;
                case 57: add  = JY_mb_tui3; break;
                case 58: add  = JY_mb_tui4; break;
                case 59: add  = JY_mb_tun; break;
                case 60: add  = JY_mb_tun2; break;
                case 61: add  = JY_mb_tun4; break;
                case 62: add  = JY_mb_tong; break;
                case 63: add  = JY_mb_tong2; break;
                case 64: add  = JY_mb_tong3; break;
                case 65: add  = JY_mb_tong4; break;
                }
                if (add != NULL)
                {
                    g_hanzi_num = (uint16_t)strlen((char *)add); //   拼音  a 的个数
                    memcpy(get_hanzi, add, g_hanzi_num);       //  把找到的汉字放在缓冲区
                    *hh         = g_hanzi_num / 2;
                    return true;
                }
            }
        }
        return false;
    }

    //首字母为 'ㄋ' (s)
    if (jy_ime_cmp == 's')
    {
        for (jy_i = 0; jy_i < sizeofN; jy_i++)                     // 16次
        {
            memcpy(jy_ime_temp, JY_index_n[jy_i], 5);              //   ai,an,ang,ao，ei,...   16个
            if (memcmp(&jy_ime_temp1[1], jy_ime_temp, 5) == 0)     // 输入的拼音和'a'的第2个韵母比较
            {
                switch (jy_i)
                {
                case 0: add  = JY_mb_na2; break;
                case 1: add  = JY_mb_na3; break;
                case 2: add  = JY_mb_na4; break;
                case 3: add  = JY_mb_na5; break;
                case 4: add  = JY_mb_ne4; break;
                case 5: add  = JY_mb_ne5; break;
                case 6: add  = JY_mb_nai3; break;
                case 7: add  = JY_mb_nai4; break;
                case 8: add  = JY_mb_nei3; break;
                case 9: add  = JY_mb_nei4; break;
                case 10: add  = JY_mb_nao2; break;
                case 11: add  = JY_mb_nao3; break;
                case 12: add  = JY_mb_nao4; break;
                case 13: add  = JY_mb_nou4; break;
                case 14: add  = JY_mb_nan; break;
                case 15: add  = JY_mb_nan2; break;
                case 16: add  = JY_mb_nan3; break;
                case 17: add  = JY_mb_nan4; break;
                case 18: add  = JY_mb_nen4; break;
                case 19: add  = JY_mb_nang2; break;
                case 20: add  = JY_mb_nang3; break;
                case 21: add  = JY_mb_neng2; break;
                case 22: add  = JY_mb_ni2; break;
                case 23: add  = JY_mb_ni3; break;
                case 24: add  = JY_mb_ni4; break;
                case 25: add  = JY_mb_nie; break;
                case 26: add  = JY_mb_nie4; break;
                case 27: add  = JY_mb_niao3; break;
                case 28: add  = JY_mb_niao4; break;
                case 29: add  = JY_mb_niu; break;
                case 30: add  = JY_mb_niu2; break;
                case 31: add  = JY_mb_niu3; break;
                case 32: add  = JY_mb_niu4; break;
                case 33: add  = JY_mb_nian; break;
                case 34: add  = JY_mb_nian2; break;
                case 35: add  = JY_mb_nian3; break;
                case 36: add  = JY_mb_nian4; break;
                case 37: add  = JY_mb_nin2; break;
                case 38: add  = JY_mb_niang2; break;
                case 39: add  = JY_mb_niang4; break;
                case 40: add  = JY_mb_ning2; break;
                case 41: add  = JY_mb_ning3; break;
                case 42: add  = JY_mb_ning4; break;
                case 43: add  = JY_mb_nu2; break;
                case 44: add  = JY_mb_nu3; break;
                case 45: add  = JY_mb_nu4; break;
                case 46: add  = JY_mb_nuo2; break;
                case 47: add  = JY_mb_nuo4; break;
                case 48: add  = JY_mb_nuan3; break;
                case 49: add  = JY_mb_nong2; break;
                case 50: add  = JY_mb_nong4; break;
                case 51: add  = JY_mb_nv3; break;
                case 52: add  = JY_mb_nue4; break;
                }
                if (add != NULL)
                {
                    g_hanzi_num = (uint16_t)strlen((char *)add); //   拼音  a 的个数
                    memcpy(get_hanzi, add, g_hanzi_num);       //  把找到的汉字放在缓冲区
                    *hh         = g_hanzi_num / 2;
                    return true;
                }
            }
        }
        return false;
    }

    //首字母为 'ㄌ' (x)
    if (jy_ime_cmp == 'x')
    {
        for (jy_i = 0; jy_i < sizeofL; jy_i++)                     // 16次
        {
            memcpy(jy_ime_temp, JY_index_l[jy_i], 5);              //   ai,an,ang,ao，ei,...   16个
            if (memcmp(&jy_ime_temp1[1], jy_ime_temp, 5) == 0)     // 输入的拼音和'a'的第2个韵母比较
            {
                switch (jy_i)
                {
                case 0: add  = JY_mb_la; break;
                case 1: add  = JY_mb_la2; break;
                case 2: add  = JY_mb_la3; break;
                case 3: add  = JY_mb_la4; break;
                case 4: add  = JY_mb_la5; break;
                case 5: add  = JY_mb_le4; break;
                case 6: add  = JY_mb_le5; break;
                case 7: add  = JY_mb_lai2; break;
                case 8: add  = JY_mb_lai4; break;
                case 9: add  = JY_mb_lei; break;
                case 10: add  = JY_mb_lei2; break;
                case 11: add  = JY_mb_lei3; break;
                case 12: add  = JY_mb_lei4; break;
                case 13: add  = JY_mb_lao; break;
                case 14: add  = JY_mb_lao2; break;
                case 15: add  = JY_mb_lao3; break;
                case 16: add  = JY_mb_lao4; break;
                case 17: add  = JY_mb_lan2; break;
                case 18: add  = JY_mb_lan3; break;
                case 19: add  = JY_mb_lan4; break;
                case 20: add  = JY_mb_lang2; break;
                case 21: add  = JY_mb_lang3; break;
                case 22: add  = JY_mb_lang4; break;
                case 23: add  = JY_mb_leng2; break;
                case 24: add  = JY_mb_leng3; break;
                case 25: add  = JY_mb_leng4; break;
                case 26: add  = JY_mb_li; break;
                case 27: add  = JY_mb_li2; break;
                case 28: add  = JY_mb_li3; break;
                case 29: add  = JY_mb_li4; break;
                case 30: add  = JY_mb_li5; break;
                case 31: add  = JY_mb_lie; break;
                case 32: add  = JY_mb_lie3; break;
                case 33: add  = JY_mb_lie4; break;
                case 34: add  = JY_mb_liao; break;
                case 35: add  = JY_mb_liao2; break;
                case 36: add  = JY_mb_liao3; break;
                case 37: add  = JY_mb_liao4; break;
                case 38: add  = JY_mb_liu; break;
                case 39: add  = JY_mb_liu2; break;
                case 40: add  = JY_mb_liu3; break;
                case 41: add  = JY_mb_liu4; break;
                case 42: add  = JY_mb_lian2; break;
                case 43: add  = JY_mb_lian3; break;
                case 44: add  = JY_mb_lian4; break;
                case 45: add  = JY_mb_lin2; break;
                case 46: add  = JY_mb_lin3; break;
                case 47: add  = JY_mb_lin4; break;
                case 48: add  = JY_mb_liang2; break;
                case 49: add  = JY_mb_liang3; break;
                case 50: add  = JY_mb_liang4; break;
                case 51: add  = JY_mb_ling2; break;
                case 52: add  = JY_mb_ling3; break;
                case 53: add  = JY_mb_ling4; break;
                case 54: add  = JY_mb_lou; break;
                case 55: add  = JY_mb_lou2; break;
                case 56: add  = JY_mb_lou3; break;
                case 57: add  = JY_mb_lou4; break;
                case 58: add  = JY_mb_lu; break;
                case 59: add  = JY_mb_lu2; break;
                case 60: add  = JY_mb_lu3; break;
                case 61: add  = JY_mb_lu4; break;
                case 62: add  = JY_mb_luo; break;
                case 63: add  = JY_mb_luo2; break;
                case 64: add  = JY_mb_luo3; break;
                case 65: add  = JY_mb_luo4; break;
                case 66: add  = JY_mb_luan2; break;
                case 67: add  = JY_mb_luan3; break;
                case 68: add  = JY_mb_luan4; break;
                case 69: add  = JY_mb_lun; break;
                case 70: add  = JY_mb_lun2; break;
                case 71: add  = JY_mb_lun4; break;
                case 72: add  = JY_mb_long; break;
                case 73: add  = JY_mb_long2; break;
                case 74: add  = JY_mb_long3; break;
                case 75: add  = JY_mb_long4; break;
                case 76: add  = JY_mb_lv2; break;
                case 77: add  = JY_mb_lv3; break;
                case 78: add  = JY_mb_lv4; break;
                case 79: add  = JY_mb_lue4; break;
                }
                if (add != NULL)
                {
                    g_hanzi_num = (uint16_t)strlen((char *)add); //   拼音  a 的个数
                    memcpy(get_hanzi, add, g_hanzi_num);       //  把找到的汉字放在缓冲区
                    *hh         = g_hanzi_num / 2;
                    return true;
                }
            }
        }
        return false;
    }

    //首字母为 'ㄍ' (e)
    if (jy_ime_cmp == 'e')
    {
        for (jy_i = 0; jy_i < sizeofG; jy_i++)                     // 16次
        {
            memcpy(jy_ime_temp, JY_index_g[jy_i], 5);              //   ai,an,ang,ao，ei,...   16个
            if (memcmp(&jy_ime_temp1[1], jy_ime_temp, 5) == 0)     // 输入的拼音和'a'的第2个韵母比较
            {
                (void)printf("jy_i==%d\n", jy_i);
                switch (jy_i)
                {
                case 0: add  = JY_mb_ga2; break;
                case 1: add  = JY_mb_ga3; break;
                case 2: add  = JY_mb_ga4; break;
                case 3: add  = JY_mb_ge; break;
                case 4: add  = JY_mb_ge2; break;
                case 5: add  = JY_mb_ge3; break;
                case 6: add  = JY_mb_ge4; break;
                case 7: add  = JY_mb_gai; break;
                case 8: add  = JY_mb_gai3; break;
                case 9: add  = JY_mb_gai4; break;
                case 10: add  = JY_mb_gei3; break;
                case 11: add  = JY_mb_gao; break;
                case 12: add  = JY_mb_gao3; break;
                case 13: add  = JY_mb_gao4; break;
                case 14: add  = JY_mb_gou; break;
                case 15: add  = JY_mb_gou3; break;
                case 16: add  = JY_mb_gou4; break;
                case 17: add  = JY_mb_gan; break;
                case 18: add  = JY_mb_gan3; break;
                case 19: add  = JY_mb_gan4; break;
                case 20: add  = JY_mb_gen; break;
                case 21: add  = JY_mb_gen4; break;
                case 22: add  = JY_mb_gang; break;
                case 23: add  = JY_mb_gang3; break;
                case 24: add  = JY_mb_gang4; break;
                case 25: add  = JY_mb_geng; break;
                case 26: add  = JY_mb_geng3; break;
                case 27: add  = JY_mb_geng4; break;
                case 28: add  = JY_mb_gu; break;
                case 29: add  = JY_mb_gu3; break;
                case 30: add  = JY_mb_gu4; break;
                case 31: add  = JY_mb_gua; break;
                case 32: add  = JY_mb_gua3; break;
                case 33: add  = JY_mb_gua4; break;
                case 34: add  = JY_mb_guo; break;
                case 35: add  = JY_mb_guo2; break;
                case 36: add  = JY_mb_guo3; break;
                case 37: add  = JY_mb_guo4; break;
                case 38: add  = JY_mb_guai; break;
                case 39: add  = JY_mb_guai3; break;
                case 40: add  = JY_mb_guai4; break;
                case 41: add  = JY_mb_gui; break;
                case 42: add  = JY_mb_gui3; break;
                case 43: add  = JY_mb_gui4; break;
                case 44: add  = JY_mb_guan; break;
                case 45: add  = JY_mb_guan3; break;
                case 46: add  = JY_mb_guan4; break;
                case 47: add  = JY_mb_gun3; break;
                case 48: add  = JY_mb_gun4; break;
                case 49: add  = JY_mb_guang; break;
                case 50: add  = JY_mb_guang3; break;
                case 51: add  = JY_mb_guang4; break;
                case 52: add  = JY_mb_gong; break;
                case 53: add  = JY_mb_gong3; break;
                case 54: add  = JY_mb_gong4; break;
                }
                if (add != NULL)
                {
                    g_hanzi_num = (uint16_t)strlen((char *)add); //   拼音  a 的个数
                    memcpy(get_hanzi, add, g_hanzi_num);       //  把找到的汉字放在缓冲区
                    *hh         = g_hanzi_num / 2;
                    return true;
                }
            }
        }
        return false;
    }

    //首字母为 'ㄎ'
    if (jy_ime_cmp == 'd')
    {
        for (jy_i = 0; jy_i < sizeofK; jy_i++)                     // 16次
        {
            memcpy(jy_ime_temp, JY_index_k[jy_i], 5);              //   ai,an,ang,ao，ei,...   16个
            if (memcmp(&jy_ime_temp1[1], jy_ime_temp, 5) == 0)     // 输入的拼音和'a'的第2个韵母比较
            {
                switch (jy_i)
                {
                case 0: add = JY_mb_ka; break;
                case 1: add = JY_mb_ka3; break;
                case 2: add = JY_mb_ke; break;
                case 3: add = JY_mb_ke2; break;
                case 4: add = JY_mb_ke3; break;
                case 5: add = JY_mb_ke4; break;
                case 6: add = JY_mb_kai; break;
                case 7: add = JY_mb_kai3; break;
                case 8: add = JY_mb_kai4; break;
                case 9: add = JY_mb_kao; break;
                case 10: add = JY_mb_kao3; break;
                case 11: add = JY_mb_kao4; break;
                case 12: add = JY_mb_kou; break;
                case 13: add = JY_mb_kou3; break;
                case 14: add = JY_mb_kou4; break;
                case 15: add = JY_mb_kan; break;
                case 16: add = JY_mb_kan3; break;
                case 17: add = JY_mb_kan4; break;
                case 18: add = JY_mb_ken3; break;
                case 19: add = JY_mb_ken4; break;
                case 20: add = JY_mb_kang; break;
                case 21: add = JY_mb_kang2; break;
                case 22: add = JY_mb_kang4; break;
                case 23: add = JY_mb_keng; break;
                case 24: add = JY_mb_ku; break;
                case 25: add = JY_mb_ku3; break;
                case 26: add = JY_mb_ku4; break;
                case 27: add = JY_mb_kua; break;
                case 28: add = JY_mb_kua3; break;
                case 29: add = JY_mb_kua4; break;
                case 30: add = JY_mb_kuo4; break;
                case 31: add = JY_mb_kuai; break;
                case 32: add = JY_mb_kuai3; break;
                case 33: add = JY_mb_kuai4; break;
                case 34: add = JY_mb_kui; break;
                case 35: add = JY_mb_kui2; break;
                case 36: add = JY_mb_kui3; break;
                case 37: add = JY_mb_kui4; break;
                case 38: add = JY_mb_kuan; break;
                case 39: add = JY_mb_kuan3; break;
                case 40: add = JY_mb_kun; break;
                case 41: add = JY_mb_kun3; break;
                case 42: add = JY_mb_kun4; break;
                case 43: add = JY_mb_kuang; break;
                case 44: add = JY_mb_kuang2; break;
                case 45: add = JY_mb_kuang4; break;
                case 46: add = JY_mb_kong; break;
                case 47: add = JY_mb_kong3; break;
                case 48: add = JY_mb_kong4; break;
                }
                if (add != NULL)
                {
                    g_hanzi_num = (uint16_t)strlen((char *)add); //   拼音  a 的个数
                    memcpy(get_hanzi, add, g_hanzi_num);       //  把找到的汉字放在缓冲区
                    *hh         = g_hanzi_num / 2;
                    return true;
                }
            }
        }
        return false;
    }

    //首字母为 'ㄏ'
    if (jy_ime_cmp == 'c')
    {
        for (jy_i = 0; jy_i < sizeofH; jy_i++)                     // 16次
        {
            memcpy(jy_ime_temp, JY_index_h[jy_i], 5);              //   ai,an,ang,ao，ei,...   16个
            if (memcmp(&jy_ime_temp1[1], jy_ime_temp, 5) == 0)     // 输入的拼音和'a'的第2个韵母比较
            {
                switch (jy_i)
                {
                case 0: add = JY_mb_ha; break;
                case 1: add = JY_mb_ha2; break;
                case 2: add = JY_mb_ha4; break;
                case 3: add = JY_mb_he; break;
                case 4: add = JY_mb_he2; break;
                case 5: add = JY_mb_he4; break;
                case 6: add = JY_mb_hai; break;
                case 7: add = JY_mb_hai2; break;
                case 8: add = JY_mb_hai3; break;
                case 9: add = JY_mb_hai4; break;
                case 10: add = JY_mb_hei; break;
                case 11: add = JY_mb_hao; break;
                case 12: add = JY_mb_hao2; break;
                case 13: add = JY_mb_hao3; break;
                case 14: add = JY_mb_hao4; break;
                case 15: add = JY_mb_hou; break;
                case 16: add = JY_mb_hou2; break;
                case 17: add = JY_mb_hou3; break;
                case 18: add = JY_mb_hou4; break;
                case 19: add = JY_mb_han; break;
                case 20: add = JY_mb_han2; break;
                case 21: add = JY_mb_han3; break;
                case 22: add = JY_mb_han4; break;
                case 23: add = JY_mb_hen2; break;
                case 24: add = JY_mb_hen3; break;
                case 25: add = JY_mb_hen4; break;
                case 26: add = JY_mb_hang; break;
                case 27: add = JY_mb_hang2; break;
                case 28: add = JY_mb_heng; break;
                case 29: add = JY_mb_heng2; break;
                case 30: add = JY_mb_heng4; break;
                case 31: add = JY_mb_hu; break;
                case 32: add = JY_mb_hu2; break;
                case 33: add = JY_mb_hu3; break;
                case 34: add = JY_mb_hu4; break;
                case 35: add = JY_mb_hua; break;
                case 36: add = JY_mb_hua2; break;
                case 37: add = JY_mb_hua4; break;
                case 38: add = JY_mb_huo; break;
                case 39: add = JY_mb_huo2; break;
                case 40: add = JY_mb_huo3; break;
                case 41: add = JY_mb_huo4; break;
                case 42: add = JY_mb_huai2; break;
                case 43: add = JY_mb_huai4; break;
                case 44: add = JY_mb_hui; break;
                case 45: add = JY_mb_hui2; break;
                case 46: add = JY_mb_hui3; break;
                case 47: add = JY_mb_hui4; break;
                case 48: add = JY_mb_huan; break;
                case 49: add = JY_mb_huan2; break;
                case 50: add = JY_mb_huan3; break;
                case 51: add = JY_mb_huan4; break;
                case 52: add = JY_mb_hun; break;
                case 53: add = JY_mb_hun2; break;
                case 54: add = JY_mb_hun4; break;
                case 55: add = JY_mb_huang; break;
                case 56: add = JY_mb_huang2; break;
                case 57: add = JY_mb_huang3; break;
                case 58: add = JY_mb_huang4; break;
                case 59: add = JY_mb_hong; break;
                case 60: add = JY_mb_hong2; break;
                case 61: add = JY_mb_hong3; break;
                case 62: add = JY_mb_hong4; break;
                }
                if (add != NULL)
                {
                    g_hanzi_num = (uint16_t)strlen((char *)add); //   拼音  a 的个数
                    memcpy(get_hanzi, add, g_hanzi_num);       //  把找到的汉字放在缓冲区
                    *hh         = g_hanzi_num / 2;
                    return true;
                }
            }
        }
        return false;
    }

    //首字母为 'ㄐ'
    if (jy_ime_cmp == 'r')
    {
        for (jy_i = 0; jy_i < sizeofJ; jy_i++)                     // 16次
        {
            memcpy(jy_ime_temp, JY_index_j[jy_i], 5);              //   ai,an,ang,ao，ei,...   16个
            if (memcmp(&jy_ime_temp1[1], jy_ime_temp, 5) == 0)     // 输入的拼音和'a'的第2个韵母比较
            {
                switch (jy_i)
                {
                case 0: add = JY_mb_ji; break;
                case 1: add = JY_mb_ji2; break;
                case 2: add = JY_mb_ji3; break;
                case 3: add = JY_mb_ji4; break;
                case 4: add = JY_mb_jia; break;
                case 5: add = JY_mb_jia2; break;
                case 6: add = JY_mb_jia3; break;
                case 7: add = JY_mb_jia4; break;
                case 8: add = JY_mb_jie; break;
                case 9: add = JY_mb_jie2; break;
                case 10: add = JY_mb_jie3; break;
                case 11: add = JY_mb_jie4; break;
                case 12: add = JY_mb_jiao; break;
                case 13: add = JY_mb_jiao2; break;
                case 14: add = JY_mb_jiao3; break;
                case 15: add = JY_mb_jiao4; break;
                case 16: add = JY_mb_jiu; break;
                case 17: add = JY_mb_jiu3; break;
                case 18: add = JY_mb_jiu4; break;
                case 19: add = JY_mb_jian; break;
                case 20: add = JY_mb_jian3; break;
                case 21: add = JY_mb_jian4; break;
                case 22: add = JY_mb_jin; break;
                case 23: add = JY_mb_jin3; break;
                case 24: add = JY_mb_jin4; break;
                case 25: add = JY_mb_jing; break;
                case 26: add = JY_mb_jing3; break;
                case 27: add = JY_mb_jing4; break;
                case 28: add = JY_mb_jiang; break;
                case 29: add = JY_mb_jiang3; break;
                case 30: add = JY_mb_jiang4; break;
                case 31: add = JY_mb_ju; break;
                case 32: add = JY_mb_ju2; break;
                case 33: add = JY_mb_ju3; break;
                case 34: add = JY_mb_ju4; break;
                case 35: add = JY_mb_jue; break;
                case 36: add = JY_mb_jue2; break;
                case 37: add = JY_mb_juan; break;
                case 38: add = JY_mb_juan3; break;
                case 39: add = JY_mb_juan4; break;
                case 40: add = JY_mb_jun; break;
                case 41: add = JY_mb_jun4; break;
                case 42: add = JY_mb_jiong; break;
                case 43: add = JY_mb_jiong3; break;
                }
                if (add != NULL)
                {
                    g_hanzi_num = (uint16_t)strlen((char *)add); //   拼音  a 的个数
                    memcpy(get_hanzi, add, g_hanzi_num);       //  把找到的汉字放在缓冲区
                    *hh         = g_hanzi_num / 2;
                    return true;
                }
            }
        }
        return false;
    }

    //首字母为 'ㄑ'
    if (jy_ime_cmp == 'f')
    {
        for (jy_i = 0; jy_i < sizeofQ; jy_i++)                     // 16次
        {
            memcpy(jy_ime_temp, JY_index_q[jy_i], 5);              //   ai,an,ang,ao，ei,...   16个
            if (memcmp(&jy_ime_temp1[1], jy_ime_temp, 5) == 0)     // 输入的拼音和'a'的第2个韵母比较
            {
                switch (jy_i)
                {
                case 0: add = JY_mb_qi; break;
                case 1: add = JY_mb_qi2; break;
                case 2: add = JY_mb_qi3; break;
                case 3: add = JY_mb_qi4; break;
                case 4: add = JY_mb_qia; break;
                case 5: add = JY_mb_qia3; break;
                case 6: add = JY_mb_qia4; break;
                case 7: add = JY_mb_qie; break;
                case 8: add = JY_mb_qie2; break;
                case 9: add = JY_mb_qie3; break;
                case 10: add = JY_mb_qie4; break;
                case 11: add = JY_mb_qiao; break;
                case 12: add = JY_mb_qiao2; break;
                case 13: add = JY_mb_qiao3; break;
                case 14: add = JY_mb_qiao4; break;
                case 15: add = JY_mb_qiu; break;
                case 16: add = JY_mb_qiu2; break;
                case 17: add = JY_mb_qiu3; break;
                case 18: add = JY_mb_qian; break;
                case 19: add = JY_mb_qian2; break;
                case 20: add = JY_mb_qian3; break;
                case 21: add = JY_mb_qian4; break;
                case 22: add = JY_mb_qin; break;
                case 23: add = JY_mb_qin2; break;
                case 24: add = JY_mb_qin3; break;
                case 25: add = JY_mb_qin4; break;
                case 26: add = JY_mb_qiang; break;
                case 27: add = JY_mb_qiang2; break;
                case 28: add = JY_mb_qiang3; break;
                case 29: add = JY_mb_qiang4; break;
                case 30: add = JY_mb_qing; break;
                case 31: add = JY_mb_qing2; break;
                case 32: add = JY_mb_qing3; break;
                case 33: add = JY_mb_qing4; break;
                case 34: add = JY_mb_qu; break;
                case 35: add = JY_mb_qu2; break;
                case 36: add = JY_mb_qu3; break;
                case 37: add = JY_mb_qu4; break;
                case 38: add = JY_mb_que; break;
                case 39: add = JY_mb_que2; break;
                case 40: add = JY_mb_que4; break;
                case 41: add = JY_mb_quan; break;
                case 42: add = JY_mb_quan2; break;
                case 43: add = JY_mb_quan3; break;
                case 44: add = JY_mb_quan4; break;
                case 45: add = JY_mb_qun; break;
                case 46: add = JY_mb_qun2; break;
                case 47: add = JY_mb_qiong; break;
                case 48: add = JY_mb_qiong2; break;
                }
                if (add != NULL)
                {
                    g_hanzi_num = (uint16_t)strlen((char *)add); //   拼音  a 的个数
                    memcpy(get_hanzi, add, g_hanzi_num);       //  把找到的汉字放在缓冲区
                    *hh         = g_hanzi_num / 2;
                    return true;
                }
            }
        }
        return false;
    }

    //首字母为 'ㄒ'
    if (jy_ime_cmp == 'v')
    {
        for (jy_i = 0; jy_i < sizeofX; jy_i++)                     // 16次
        {
            memcpy(jy_ime_temp, JY_index_x[jy_i], 5);              //   ai,an,ang,ao，ei,...   16个
            if (memcmp(&jy_ime_temp1[1], jy_ime_temp, 5) == 0)     // 输入的拼音和'a'的第2个韵母比较
            {
                switch (jy_i)
                {
                case 0: add = JY_mb_xi; break;
                case 1: add = JY_mb_xi2; break;
                case 2: add = JY_mb_xi3; break;
                case 3: add = JY_mb_xi4; break;
                case 4: add = JY_mb_xia; break;
                case 5: add = JY_mb_xia2; break;
                case 6: add = JY_mb_xia4; break;
                case 7: add = JY_mb_xie; break;
                case 8: add = JY_mb_xie2; break;
                case 9: add = JY_mb_xie3; break;
                case 10: add = JY_mb_xie4; break;
                case 11: add = JY_mb_xiao; break;
                case 12: add = JY_mb_xiao3; break;
                case 13: add = JY_mb_xiao4; break;
                case 14: add = JY_mb_xiu; break;
                case 15: add = JY_mb_xiu3; break;
                case 16: add = JY_mb_xiu4; break;
                case 17: add = JY_mb_xian; break;
                case 18: add = JY_mb_xian2; break;
                case 19: add = JY_mb_xian3; break;
                case 20: add = JY_mb_xian4; break;
                case 21: add = JY_mb_xin; break;
                case 22: add = JY_mb_xin2; break;
                case 23: add = JY_mb_xin3; break;
                case 24: add = JY_mb_xin4; break;
                case 25: add = JY_mb_xiang; break;
                case 26: add = JY_mb_xiang2; break;
                case 27: add = JY_mb_xiang3; break;
                case 28: add = JY_mb_xiang4; break;
                case 29: add = JY_mb_xing; break;
                case 30: add = JY_mb_xing2; break;
                case 31: add = JY_mb_xing3; break;
                case 32: add = JY_mb_xing4; break;
                case 33: add = JY_mb_xu; break;
                case 34: add = JY_mb_xu2; break;
                case 35: add = JY_mb_xu3; break;
                case 36: add = JY_mb_xu4; break;
                case 37: add = JY_mb_xue; break;
                case 38: add = JY_mb_xue2; break;
                case 39: add = JY_mb_xue3; break;
                case 40: add = JY_mb_xue4; break;
                case 41: add = JY_mb_xuan; break;
                case 42: add = JY_mb_xuan2; break;
                case 43: add = JY_mb_xuan3; break;
                case 44: add = JY_mb_xuan4; break;
                case 45: add = JY_mb_xun; break;
                case 46: add = JY_mb_xun2; break;
                case 47: add = JY_mb_xun4; break;
                case 48: add = JY_mb_xiong; break;
                case 49: add = JY_mb_xiong2; break;
                case 50: add = JY_mb_xiong4; break;
                }
                if (add != NULL)
                {
                    g_hanzi_num = (uint16_t)strlen((char *)add); //   拼音  a 的个数
                    memcpy(get_hanzi, add, g_hanzi_num);       //  把找到的汉字放在缓冲区
                    *hh         = g_hanzi_num / 2;
                    return true;
                }
            }
        }
        return false;
    }

    //首字母为 'ㄓ'
    if (jy_ime_cmp == '5')
    {
        for (jy_i = 0; jy_i < sizeofZH; jy_i++)                     // 16次
        {
            memcpy(jy_ime_temp, JY_index_zh[jy_i], 5);              //   ai,an,ang,ao，ei,...   16个
            if (memcmp(&jy_ime_temp1[1], jy_ime_temp, 5) == 0)      // 输入的拼音和'a'的第2个韵母比较
            {
                switch (jy_i)
                {
                case 0: add = JY_mb_zhi; break;
                case 1: add = JY_mb_zhi2; break;
                case 2: add = JY_mb_zhi3; break;
                case 3: add = JY_mb_zhi4; break;
                case 4: add = JY_mb_zha; break;
                case 5: add = JY_mb_zha2; break;
                case 6: add = JY_mb_zha3; break;
                case 7: add = JY_mb_zha4; break;
                case 8: add = JY_mb_zhe; break;
                case 9: add = JY_mb_zhe2; break;
                case 10: add = JY_mb_zhe3; break;
                case 11: add = JY_mb_zhe4; break;
                case 12: add = JY_mb_zhai; break;
                case 13: add = JY_mb_zhai2; break;
                case 14: add = JY_mb_zhai3; break;
                case 15: add = JY_mb_zhai4; break;
                case 16: add = JY_mb_zhao; break;
                case 17: add = JY_mb_zhao2; break;
                case 18: add = JY_mb_zhao3; break;
                case 19: add = JY_mb_zhao4; break;
                case 20: add = JY_mb_zhou; break;
                case 21: add = JY_mb_zhou2; break;
                case 22: add = JY_mb_zhou3; break;
                case 23: add = JY_mb_zhou4; break;
                case 24: add = JY_mb_zhan; break;
                case 25: add = JY_mb_zhan3; break;
                case 26: add = JY_mb_zhan4; break;
                case 27: add = JY_mb_zhen; break;
                case 28: add = JY_mb_zhen3; break;
                case 29: add = JY_mb_zhen4; break;
                case 30: add = JY_mb_zhang; break;
                case 31: add = JY_mb_zhang3; break;
                case 32: add = JY_mb_zhang4; break;
                case 33: add = JY_mb_zheng; break;
                case 34: add = JY_mb_zheng3; break;
                case 35: add = JY_mb_zheng4; break;
                case 36: add = JY_mb_zhu; break;
                case 37: add = JY_mb_zhu2; break;
                case 38: add = JY_mb_zhu3; break;
                case 39: add = JY_mb_zhu4; break;
                case 40: add = JY_mb_zhua; break;
                case 41: add = JY_mb_zhua3; break;
                case 42: add = JY_mb_zhuo; break;
                case 43: add = JY_mb_zhuo2; break;
                case 44: add = JY_mb_zhuai; break;
                case 45: add = JY_mb_zhuai3; break;
                case 46: add = JY_mb_zhuai4; break;
                case 47: add = JY_mb_zhuang; break;
                case 48: add = JY_mb_zhuang4; break;
                case 49: add = JY_mb_zhui; break;
                case 50: add = JY_mb_zhui4; break;
                case 51: add = JY_mb_zhuan; break;
                case 52: add = JY_mb_zhuan3; break;
                case 53: add = JY_mb_zhuan4; break;
                case 54: add = JY_mb_zhun; break;
                case 55: add = JY_mb_zhun3; break;
                case 56: add = JY_mb_zhong; break;
                case 57: add = JY_mb_zhong3; break;
                case 58: add = JY_mb_zhong4; break;
                }
                if (add != NULL)
                {
                    g_hanzi_num = (uint16_t)strlen((char *)add); //   拼音  a 的个数
                    memcpy(get_hanzi, add, g_hanzi_num);       //  把找到的汉字放在缓冲区
                    *hh         = g_hanzi_num / 2;
                    return true;
                }
            }
        }
        return false;
    }

    //首字母为 'ㄔ'
    if (jy_ime_cmp == 't')
    {
        for (jy_i = 0; jy_i < sizeofCH; jy_i++)                     // 16次
        {
            memcpy(jy_ime_temp, JY_index_ch[jy_i], 5);              //   ai,an,ang,ao，ei,...   16个
            if (memcmp(&jy_ime_temp1[1], jy_ime_temp, 5) == 0)      // 输入的拼音和'a'的第2个韵母比较
            {
                switch (jy_i)
                {
                case 0: add = JY_mb_chi; break;
                case 1: add = JY_mb_chi2; break;
                case 2: add = JY_mb_chi3; break;
                case 3: add = JY_mb_chi4; break;
                case 4: add = JY_mb_cha; break;
                case 5: add = JY_mb_cha2; break;
                case 6: add = JY_mb_cha3; break;
                case 7: add = JY_mb_cha4; break;
                case 8: add = JY_mb_che; break;
                case 9: add = JY_mb_che3; break;
                case 10: add = JY_mb_che4; break;
                case 11: add = JY_mb_chai; break;
                case 12: add = JY_mb_chai2; break;
                case 13: add = JY_mb_chai4; break;
                case 14: add = JY_mb_chao; break;
                case 15: add = JY_mb_chao2; break;
                case 16: add = JY_mb_chao3; break;
                case 17: add = JY_mb_chou; break;
                case 18: add = JY_mb_chou2; break;
                case 19: add = JY_mb_chou3; break;
                case 20: add = JY_mb_chou4; break;
                case 21: add = JY_mb_chan; break;
                case 22: add = JY_mb_chan2; break;
                case 23: add = JY_mb_chan3; break;
                case 24: add = JY_mb_chan4; break;
                case 25: add = JY_mb_chen; break;
                case 26: add = JY_mb_chen2; break;
                case 27: add = JY_mb_chen4; break;
                case 28: add = JY_mb_chang; break;
                case 29: add = JY_mb_chang2; break;
                case 30: add = JY_mb_chang3; break;
                case 31: add = JY_mb_chang4; break;
                case 32: add = JY_mb_cheng; break;
                case 33: add = JY_mb_cheng2; break;
                case 34: add = JY_mb_cheng3; break;
                case 35: add = JY_mb_cheng4; break;
                case 36: add = JY_mb_chu; break;
                case 37: add = JY_mb_chu2; break;
                case 38: add = JY_mb_chu3; break;
                case 39: add = JY_mb_chu4; break;
                case 40: add = JY_mb_chuo; break;
                case 41: add = JY_mb_chuo4; break;
                case 42: add = JY_mb_chuai3; break;
                case 43: add = JY_mb_chuai4; break;
                case 44: add = JY_mb_chui; break;
                case 45: add = JY_mb_chui2; break;
                case 46: add = JY_mb_chuan; break;
                case 47: add = JY_mb_chuan2; break;
                case 48: add = JY_mb_chuan3; break;
                case 49: add = JY_mb_chuan4; break;
                case 50: add = JY_mb_chuang; break;
                case 51: add = JY_mb_chuang2; break;
                case 52: add = JY_mb_chuang3; break;
                case 53: add = JY_mb_chuang4; break;
                case 54: add = JY_mb_chun; break;
                case 55: add = JY_mb_chun2; break;
                case 56: add = JY_mb_chun3; break;
                case 57: add = JY_mb_chong; break;
                case 58: add = JY_mb_chong2; break;
                case 59: add = JY_mb_chong3; break;
                case 60: add = JY_mb_chong4; break;
                }
                if (add != NULL)
                {
                    g_hanzi_num = (uint16_t)strlen((char *)add); //   拼音  a 的个数
                    memcpy(get_hanzi, add, g_hanzi_num);       //  把找到的汉字放在缓冲区
                    *hh         = g_hanzi_num / 2;
                    return true;
                }
            }
        }
        return false;
    }

    //首字母为 'ㄕ'
    if (jy_ime_cmp == 'g')
    {
        for (jy_i = 0; jy_i < sizeofSH; jy_i++)                     // 16次
        {
            memcpy(jy_ime_temp, JY_index_sh[jy_i], 5);              //   ai,an,ang,ao，ei,...   16个
            if (memcmp(&jy_ime_temp1[1], jy_ime_temp, 5) == 0)      // 输入的拼音和'a'的第2个韵母比较
            {
                switch (jy_i)
                {
                case 0: add = JY_mb_shi; break;
                case 1: add = JY_mb_shi2; break;
                case 2: add = JY_mb_shi3; break;
                case 3: add = JY_mb_shi4; break;
                case 4: add = JY_mb_sha; break;
                case 5: add = JY_mb_sha2; break;
                case 6: add = JY_mb_sha3; break;
                case 7: add = JY_mb_sha4; break;
                case 8: add = JY_mb_she; break;
                case 9: add = JY_mb_she2; break;
                case 10: add = JY_mb_she3; break;
                case 11: add = JY_mb_she4; break;
                case 12: add = JY_mb_shai; break;
                case 13: add = JY_mb_shai3; break;
                case 14: add = JY_mb_shai4; break;
                case 15: add = JY_mb_shei2; break;
                case 16: add = JY_mb_shao; break;
                case 17: add = JY_mb_shao2; break;
                case 18: add = JY_mb_shao3; break;
                case 19: add = JY_mb_shao4; break;
                case 20: add = JY_mb_shou; break;
                case 21: add = JY_mb_shou3; break;
                case 22: add = JY_mb_shou4; break;
                case 23: add = JY_mb_shan; break;
                case 24: add = JY_mb_shan3; break;
                case 25: add = JY_mb_shan4; break;
                case 26: add = JY_mb_shen; break;
                case 27: add = JY_mb_shen2; break;
                case 28: add = JY_mb_shen3; break;
                case 29: add = JY_mb_shen4; break;
                case 30: add = JY_mb_shang; break;
                case 31: add = JY_mb_shang3; break;
                case 32: add = JY_mb_shang4; break;
                case 33: add = JY_mb_sheng; break;
                case 34: add = JY_mb_sheng2; break;
                case 35: add = JY_mb_sheng3; break;
                case 36: add = JY_mb_sheng4; break;
                case 37: add = JY_mb_shu; break;
                case 38: add = JY_mb_shu2; break;
                case 39: add = JY_mb_shu3; break;
                case 40: add = JY_mb_shu4; break;
                case 41: add = JY_mb_shua; break;
                case 42: add = JY_mb_shua3; break;
                case 43: add = JY_mb_shua4; break;
                case 44: add = JY_mb_shuo; break;
                case 45: add = JY_mb_shuo4; break;
                case 46: add = JY_mb_shuai; break;
                case 47: add = JY_mb_shuai3; break;
                case 48: add = JY_mb_shuai4; break;
                case 49: add = JY_mb_shui2; break;
                case 50: add = JY_mb_shui3; break;
                case 51: add = JY_mb_shui4; break;
                case 52: add = JY_mb_shuan; break;
                case 53: add = JY_mb_shuan4; break;
                case 54: add = JY_mb_shun3; break;
                case 55: add = JY_mb_shun4; break;
                case 56: add = JY_mb_shuang; break;
                case 57: add = JY_mb_shuang3; break;
                }
                if (add != NULL)
                {
                    g_hanzi_num = (uint16_t)strlen((char *)add); //   拼音  a 的个数
                    memcpy(get_hanzi, add, g_hanzi_num);       //  把找到的汉字放在缓冲区
                    *hh         = g_hanzi_num / 2;
                    return true;
                }
            }
        }
        return false;
    }

    //首字母为 'ㄖ'
    if (jy_ime_cmp == 'b')
    {
        for (jy_i = 0; jy_i < sizeofR; jy_i++)                     // 16次
        {
            memcpy(jy_ime_temp, JY_index_r[jy_i], 5);              //   ai,an,ang,ao，ei,...   16个
            if (memcmp(&jy_ime_temp1[1], jy_ime_temp, 5) == 0)     // 输入的拼音和'a'的第2个韵母比较
            {
                switch (jy_i)
                {
                case 0: add = JY_mb_ri4; break;
                case 1: add = JY_mb_re3; break;
                case 2: add = JY_mb_re4; break;
                case 3: add = JY_mb_rao2; break;
                case 4: add = JY_mb_rao3; break;
                case 5: add = JY_mb_rao4; break;
                case 6: add = JY_mb_rou2; break;
                case 7: add = JY_mb_rou4; break;
                case 8: add = JY_mb_ran2; break;
                case 9: add = JY_mb_ran3; break;
                case 10: add = JY_mb_ren2; break;
                case 11: add = JY_mb_ren3; break;
                case 12: add = JY_mb_ren4; break;
                case 13: add = JY_mb_rang2; break;
                case 14: add = JY_mb_rang3; break;
                case 15: add = JY_mb_rang4; break;
                case 16: add = JY_mb_reng; break;
                case 17: add = JY_mb_reng2; break;
                case 18: add = JY_mb_ru2; break;
                case 19: add = JY_mb_ru3; break;
                case 20: add = JY_mb_ru4; break;
                case 21: add = JY_mb_ruo4; break;
                case 22: add = JY_mb_rui2; break;
                case 23: add = JY_mb_rui3; break;
                case 24: add = JY_mb_rui4; break;
                case 25: add = JY_mb_ruan2; break;
                case 26: add = JY_mb_ruan3; break;
                case 27: add = JY_mb_run2; break;
                case 28: add = JY_mb_run4; break;
                case 29: add = JY_mb_rong2; break;
                case 30: add = JY_mb_rong3; break;
                }
                if (add != NULL)
                {
                    g_hanzi_num = (uint16_t)strlen((char *)add); //   拼音  a 的个数
                    memcpy(get_hanzi, add, g_hanzi_num);       //  把找到的汉字放在缓冲区
                    *hh         = g_hanzi_num / 2;
                    return true;
                }
            }
        }
        return false;
    }

    // 首字母为 'ㄗ'

    if (jy_ime_cmp == 'y')
    {
        for (jy_i = 0; jy_i < sizeofZ; jy_i++)                     // 16次
        {
            memcpy(jy_ime_temp, JY_index_z[jy_i], 5);              //   ai,an,ang,ao，ei,...   16个
            if (memcmp(&jy_ime_temp1[1], jy_ime_temp, 5) == 0)     // 输入的拼音和'a'的第2个韵母比较
            {
                (void)printf("jy_i==%d\n", jy_i);
                switch (jy_i)
                {
                case 0: add = JY_mb_zi; break;
                case 1: add = JY_mb_zi3; break;
                case 2: add = JY_mb_zi4; break;
                case 3: add = JY_mb_zi5; break;
                case 4: add = JY_mb_za; break;
                case 5: add = JY_mb_za2; break;
                case 6: add = JY_mb_ze2; break;
                case 7: add = JY_mb_ze3; break;
                case 8: add = JY_mb_ze4; break;
                case 9: add = JY_mb_zai; break;
                case 10: add = JY_mb_zai3; break;
                case 11: add = JY_mb_zai4; break;
                case 12: add = JY_mb_zao; break;
                case 13: add = JY_mb_zao2; break;
                case 14: add = JY_mb_zao3; break;
                case 15: add = JY_mb_zao4; break;
                case 16: add = JY_mb_zei2; break;
                case 17: add = JY_mb_zou; break;
                case 18: add = JY_mb_zou3; break;
                case 19: add = JY_mb_zou4; break;
                case 20: add = JY_mb_zan; break;
                case 21: add = JY_mb_zan2; break;
                case 22: add = JY_mb_zan3; break;
                case 23: add = JY_mb_zan4; break;
                case 24: add = JY_mb_zen3; break;
                case 25: add = JY_mb_zen4; break;
                case 26: add = JY_mb_zang; break;
                case 27: add = JY_mb_zang3; break;
                case 28: add = JY_mb_zang4; break;
                case 29: add = JY_mb_zeng; break;
                case 30: add = JY_mb_zeng4; break;
                case 31: add = JY_mb_zu; break;
                case 32: add = JY_mb_zu2; break;
                case 33: add = JY_mb_zu3; break;
                case 34: add = JY_mb_zuo; break;
                case 35: add = JY_mb_zuo2; break;
                case 36: add = JY_mb_zuo3; break;
                case 37: add = JY_mb_zuo4; break;
                case 38: add = JY_mb_zui3; break;
                case 39: add = JY_mb_zui4; break;
                case 40: add = JY_mb_zun; break;
                case 41: add = JY_mb_zun3; break;
                case 42: add = JY_mb_zuan; break;
                case 43: add = JY_mb_zuan3; break;
                case 44: add = JY_mb_zuan4; break;
                case 45: add = JY_mb_zong; break;
                case 46: add = JY_mb_zong3; break;
                case 47: add = JY_mb_zong4; break;
                }
                if (add != NULL)
                {
                    g_hanzi_num = (uint16_t)(strlen((char *)add)); //拼音a的个数
                    memcpy(get_hanzi, add, g_hanzi_num);         //把找到的汉字放在缓冲区
                    *hh         = g_hanzi_num / 2;
                    (void)printf("HZ NUMBER=%d\n", g_hanzi_num / 2);
                    return true;
                }
            }
        }
        return false;
    }

    // 首字母为 'ㄘ'

    if (jy_ime_cmp == 'h')
    {
        for (jy_i = 0; jy_i < sizeofC; jy_i++)                     // 16次
        {
            memcpy(jy_ime_temp, JY_index_c[jy_i], 5);              //   ai,an,ang,ao，ei,...   16个
            if (memcmp(&jy_ime_temp1[1], jy_ime_temp, 5) == 0)     // 输入的拼音和'a'的第2个韵母比较
            {
                (void)printf("jy_i==%d\n", jy_i);
                switch (jy_i)
                {
                case 0: add = JY_mb_ci; break;
                case 1: add = JY_mb_ci2; break;
                case 2: add = JY_mb_ci3; break;
                case 3: add = JY_mb_ci4; break;
                case 4: add = JY_mb_ca; break;
                case 5: add = JY_mb_ce4; break;
                case 6: add = JY_mb_cai; break;
                case 7: add = JY_mb_cai2; break;
                case 8: add = JY_mb_cai3; break;
                case 9: add = JY_mb_cai4; break;
                case 10: add = JY_mb_cao; break;
                case 11: add = JY_mb_cao2; break;
                case 12: add = JY_mb_cao3; break;
                case 13: add = JY_mb_cou4; break;
                case 14: add = JY_mb_can; break;
                case 15: add = JY_mb_can2; break;
                case 16: add = JY_mb_can3; break;
                case 17: add = JY_mb_can4; break;
                case 18: add = JY_mb_cen; break;
                case 19: add = JY_mb_cen2; break;
                case 20: add = JY_mb_cang; break;
                case 21: add = JY_mb_cang2; break;
                case 22: add = JY_mb_ceng; break;
                case 23: add = JY_mb_ceng2; break;
                case 24: add = JY_mb_ceng4; break;
                case 25: add = JY_mb_cu; break;
                case 26: add = JY_mb_cu2; break;
                case 27: add = JY_mb_cu4; break;
                case 28: add = JY_mb_cuo; break;
                case 29: add = JY_mb_cuo2; break;
                case 30: add = JY_mb_cuo3; break;
                case 31: add = JY_mb_cuo4; break;
                case 32: add = JY_mb_cui; break;
                case 33: add = JY_mb_cui3; break;
                case 34: add = JY_mb_cui4; break;
                case 35: add = JY_mb_cuan; break;
                case 36: add = JY_mb_cuan2; break;
                case 37: add = JY_mb_cuan4; break;
                case 38: add = JY_mb_cun; break;
                case 39: add = JY_mb_cun2; break;
                case 40: add = JY_mb_cun3; break;
                case 41: add = JY_mb_cun4; break;
                case 42: add = JY_mb_cong; break;
                case 43: add = JY_mb_cong2; break;
                }
                if (add != NULL)
                {
                    g_hanzi_num = (uint16_t)(strlen((char *)add)); //拼音a的个数
                    memcpy(get_hanzi, add, g_hanzi_num);         //把找到的汉字放在缓冲区
                    *hh         = g_hanzi_num / 2;
                    (void)printf("HZ NUMBER=%d\n", g_hanzi_num / 2);
                    return true;
                }
            }
        }
        return false;
    }

    // 首字母为 'ㄙ'

    if (jy_ime_cmp == 'n')
    {
        for (jy_i = 0; jy_i < sizeofS; jy_i++)                     // 16次
        {
            memcpy(jy_ime_temp, JY_index_s[jy_i], 5);              //   ai,an,ang,ao，ei,...   16个
            if (memcmp(&jy_ime_temp1[1], jy_ime_temp, 5) == 0)     // 输入的拼音和'a'的第2个韵母比较
            {
                (void)printf("jy_i==%d\n", jy_i);
                switch (jy_i)
                {
                case 0: add = JY_mb_si; break;
                case 1: add = JY_mb_si3; break;
                case 2: add = JY_mb_si4; break;
                case 3: add = JY_mb_sa; break;
                case 4: add = JY_mb_sa3; break;
                case 5: add = JY_mb_sa4; break;
                case 6: add = JY_mb_se4; break;
                case 7: add = JY_mb_sai; break;
                case 8: add = JY_mb_sai4; break;
                case 9: add = JY_mb_san; break;
                case 10: add = JY_mb_san3; break;
                case 11: add = JY_mb_san4; break;
                case 12: add = JY_mb_sao; break;
                case 13: add = JY_mb_sao3; break;
                case 14: add = JY_mb_sao4; break;
                case 15: add = JY_mb_sou; break;
                case 16: add = JY_mb_sou3; break;
                case 17: add = JY_mb_sou4; break;
                case 18: add = JY_mb_sen; break;
                case 19: add = JY_mb_sang; break;
                case 20: add = JY_mb_sang3; break;
                case 21: add = JY_mb_sang4; break;
                case 22: add = JY_mb_seng; break;
                case 23: add = JY_mb_su; break;
                case 24: add = JY_mb_su2; break;
                case 25: add = JY_mb_su4; break;
                case 26: add = JY_mb_suo; break;
                case 27: add = JY_mb_suo3; break;
                case 28: add = JY_mb_sui; break;
                case 29: add = JY_mb_sui2; break;
                case 30: add = JY_mb_sui3; break;
                case 31: add = JY_mb_sui4; break;
                case 32: add = JY_mb_suan; break;
                case 33: add = JY_mb_suan4; break;
                case 34: add = JY_mb_sun; break;
                case 35: add = JY_mb_sun3; break;
                case 36: add = JY_mb_song; break;
                case 37: add = JY_mb_song3; break;
                case 38: add = JY_mb_song4; break;
                }
                if (add != NULL)
                {
                    g_hanzi_num = (uint16_t)(strlen((char *)add)); //拼音a的个数
                    memcpy(get_hanzi, add, g_hanzi_num);         //把找到的汉字放在缓冲区
                    *hh         = g_hanzi_num / 2;
                    (void)printf("HZ NUMBER=%d\n", g_hanzi_num / 2);
                    return true;
                }
            }
        }
        return false;
    }

    // 首字母为 'ㄚ'

    if (jy_ime_cmp == '8')
    {
        for (jy_i = 0; jy_i < sizeofA; jy_i++)                     // 16次
        {
            memcpy(jy_ime_temp, JY_index_a[jy_i], 5);              //   ai,an,ang,ao，ei,...   16个
            if (memcmp(&jy_ime_temp1[1], jy_ime_temp, 5) == 0)     // 输入的拼音和'a'的第2个韵母比较
            {
                (void)printf("jy_i==%d\n", jy_i);
                switch (jy_i)
                {
                case 0: add = JY_mb_a; break;
                case 1: add = JY_mb_a2; break;
                case 2: add = JY_mb_a3; break;
                case 3: add = JY_mb_a4; break;
                case 4: add = JY_mb_a5; break;
                }
                if (add != NULL)
                {
                    g_hanzi_num = (uint16_t)(strlen((char *)add)); //拼音a的个数
                    memcpy(get_hanzi, add, g_hanzi_num);         //把找到的汉字放在缓冲区
                    *hh         = g_hanzi_num / 2;
                    (void)printf("HZ NUMBER=%d\n", g_hanzi_num / 2);
                    return true;
                }
            }
        }
        return false;
    }

    // 首字母为 'ㄛ'

    if (jy_ime_cmp == 'i')
    {
        for (jy_i = 0; jy_i < sizeofO; jy_i++)                     // 16次
        {
            memcpy(jy_ime_temp, JY_index_o[jy_i], 5);              //   ai,an,ang,ao，ei,...   16个
            if (memcmp(&jy_ime_temp1[1], jy_ime_temp, 5) == 0)     // 输入的拼音和'a'的第2个韵母比较
            {
                (void)printf("jy_i==%d\n", jy_i);
                switch (jy_i)
                {
                case 0: add = JY_mb_o; break;
                case 1: add = JY_mb_o2; break;
                case 2: add = JY_mb_o3; break;
                }
                if (add != NULL)
                {
                    g_hanzi_num = (uint16_t)(strlen((char *)add)); //拼音a的个数
                    memcpy(get_hanzi, add, g_hanzi_num);         //把找到的汉字放在缓冲区
                    *hh         = g_hanzi_num / 2;
                    (void)printf("HZ NUMBER=%d\n", g_hanzi_num / 2);
                    return true;
                }
            }
        }
        return false;
    }

    // 首字母为 'ㄜ'

    if (jy_ime_cmp == 'k')
    {
        for (jy_i = 0; jy_i < sizeofE; jy_i++)                     // 16次
        {
            memcpy(jy_ime_temp, JY_index_e[jy_i], 5);              //   ai,an,ang,ao，ei,...   16个
            if (memcmp(&jy_ime_temp1[1], jy_ime_temp, 5) == 0)     // 输入的拼音和'a'的第2个韵母比较
            {
                (void)printf("jy_i==%d\n", jy_i);
                switch (jy_i)
                {
                case 0: add = JY_mb_e; break;
                case 1: add = JY_mb_e2; break;
                case 2: add = JY_mb_e3; break;
                case 3: add = JY_mb_e4; break;
                }
                if (add != NULL)
                {
                    g_hanzi_num = (uint16_t)(strlen((char *)add)); //拼音a的个数
                    memcpy(get_hanzi, add, g_hanzi_num);         //把找到的汉字放在缓冲区
                    *hh         = g_hanzi_num / 2;
                    (void)printf("HZ NUMBER=%d\n", g_hanzi_num / 2);
                    return true;
                }
            }
        }
        return false;
    }

    // 首字母为 'ㄝ ㄟ'

    if (jy_ime_cmp == ',' || jy_ime_cmp == 'o')
    {
        for (jy_i = 0; jy_i < sizeofEI; jy_i++)                     // 16次
        {
            memcpy(jy_ime_temp, JY_index_ei[jy_i], 5);              //   ai,an,ang,ao，ei,...   16个
            if (memcmp(&jy_ime_temp1[1], jy_ime_temp, 5) == 0)      // 输入的拼音和'a'的第2个韵母比较
            {
                (void)printf("jy_i==%d\n", jy_i);
                switch (jy_i)
                {
                case 0: add = JY_mb_ei4; break;
                }
                if (add != NULL)
                {
                    g_hanzi_num = (uint16_t)(strlen((char *)add)); //拼音a的个数
                    memcpy(get_hanzi, add, g_hanzi_num);         //把找到的汉字放在缓冲区
                    *hh         = g_hanzi_num / 2;
                    (void)printf("HZ NUMBER=%d\n", g_hanzi_num / 2);
                    return true;
                }
            }
        }
        return false;
    }

    // 首字母为 'ㄞ'

    if (jy_ime_cmp == '9')
    {
        for (jy_i = 0; jy_i < sizeofAI; jy_i++)                     // 16次
        {
            memcpy(jy_ime_temp, JY_index_ai[jy_i], 5);              //   ai,an,ang,ao，ei,...   16个
            if (memcmp(&jy_ime_temp1[1], jy_ime_temp, 5) == 0)      // 输入的拼音和'a'的第2个韵母比较
            {
                (void)printf("jy_i==%d\n", jy_i);
                switch (jy_i)
                {
                case 0: add = JY_mb_ai; break;
                case 1: add = JY_mb_ai2; break;
                case 2: add = JY_mb_ai3; break;
                case 3: add = JY_mb_ai4; break;
                }
                if (add != NULL)
                {
                    g_hanzi_num = (uint16_t)(strlen((char *)add)); //拼音a的个数
                    memcpy(get_hanzi, add, g_hanzi_num);         //把找到的汉字放在缓冲区
                    *hh         = g_hanzi_num / 2;
                    (void)printf("HZ NUMBER=%d\n", g_hanzi_num / 2);
                    return true;
                }
            }
        }
        return false;
    }

    // 首字母为 'ㄠ'

    if (jy_ime_cmp == 'l')
    {
        for (jy_i = 0; jy_i < sizeofAO; jy_i++)                     // 16次
        {
            memcpy(jy_ime_temp, JY_index_ao[jy_i], 5);              //   ai,an,ang,ao，ei,...   16个
            if (memcmp(&jy_ime_temp1[1], jy_ime_temp, 5) == 0)      // 输入的拼音和'a'的第2个韵母比较
            {
                (void)printf("jy_i==%d\n", jy_i);
                switch (jy_i)
                {
                case 0: add = JY_mb_ao; break;
                case 1: add = JY_mb_ao2; break;
                case 2: add = JY_mb_ao3; break;
                case 3: add = JY_mb_ao4; break;
                }
                if (add != NULL)
                {
                    g_hanzi_num = (uint16_t)(strlen((char *)add)); //拼音a的个数
                    memcpy(get_hanzi, add, g_hanzi_num);         //把找到的汉字放在缓冲区
                    *hh         = g_hanzi_num / 2;
                    (void)printf("HZ NUMBER=%d\n", g_hanzi_num / 2);
                    return true;
                }
            }
        }
        return false;
    }

    // 首字母为 'ㄡ'

    if (jy_ime_cmp == '.')
    {
        for (jy_i = 0; jy_i < sizeofOU; jy_i++)                     // 16次
        {
            memcpy(jy_ime_temp, JY_index_ou[jy_i], 5);              //   ai,an,ang,ao，ei,...   16个
            if (memcmp(&jy_ime_temp1[1], jy_ime_temp, 5) == 0)      // 输入的拼音和'a'的第2个韵母比较
            {
                (void)printf("jy_i==%d\n", jy_i);
                switch (jy_i)
                {
                case 0: add = JY_mb_ou; break;
                case 1: add = JY_mb_ou3; break;
                case 2: add = JY_mb_ou4; break;
                }
                if (add != NULL)
                {
                    g_hanzi_num = (uint16_t)(strlen((char *)add)); //拼音a的个数
                    memcpy(get_hanzi, add, g_hanzi_num);         //把找到的汉字放在缓冲区
                    *hh         = g_hanzi_num / 2;
                    (void)printf("HZ NUMBER=%d\n", g_hanzi_num / 2);
                    return true;
                }
            }
        }
        return false;
    }

    // 首字母为 'ㄢ'

    if (jy_ime_cmp == '0')
    {
        for (jy_i = 0; jy_i < sizeofAN; jy_i++)                     // 16次
        {
            memcpy(jy_ime_temp, JY_index_an[jy_i], 5);              //   ai,an,ang,ao，ei,...   16个
            if (memcmp(&jy_ime_temp1[1], jy_ime_temp, 5) == 0)      // 输入的拼音和'a'的第2个韵母比较
            {
                (void)printf("jy_i==%d\n", jy_i);
                switch (jy_i)
                {
                case 0: add = JY_mb_an; break;
                case 1: add = JY_mb_an3; break;
                case 2: add = JY_mb_an4; break;
                }
                if (add != NULL)
                {
                    g_hanzi_num = (uint16_t)(strlen((char *)add)); //拼音a的个数
                    memcpy(get_hanzi, add, g_hanzi_num);         //把找到的汉字放在缓冲区
                    *hh         = g_hanzi_num / 2;
                    (void)printf("HZ NUMBER=%d\n", g_hanzi_num / 2);
                    return true;
                }
            }
        }
        return false;
    }

    // 首字母为 'ㄣ'

    if (jy_ime_cmp == 'p')
    {
        for (jy_i = 0; jy_i < sizeofEN; jy_i++)                     // 16次
        {
            memcpy(jy_ime_temp, JY_index_en[jy_i], 5);              //   ai,an,ang,ao，ei,...   16个
            if (memcmp(&jy_ime_temp1[1], jy_ime_temp, 5) == 0)      // 输入的拼音和'a'的第2个韵母比较
            {
                (void)printf("jy_i==%d\n", jy_i);
                switch (jy_i)
                {
                case 0: add = JY_mb_en; break;
                case 1: add = JY_mb_en4; break;
                }
                if (add != NULL)
                {
                    g_hanzi_num = (uint16_t)(strlen((char *)add)); //拼音a的个数
                    memcpy(get_hanzi, add, g_hanzi_num);         //把找到的汉字放在缓冲区
                    *hh         = g_hanzi_num / 2;
                    (void)printf("HZ NUMBER=%d\n", g_hanzi_num / 2);
                    return true;
                }
            }
        }
        return false;
    }

    // 首字母为 'ㄤ'

    if (jy_ime_cmp == ';')
    {
        for (jy_i = 0; jy_i < sizeofANG; jy_i++)                     // 16次
        {
            memcpy(jy_ime_temp, JY_index_ang[jy_i], 5);              //   ai,an,ang,ao，ei,...   16个
            if (memcmp(&jy_ime_temp1[1], jy_ime_temp, 5) == 0)       // 输入的拼音和'a'的第2个韵母比较
            {
                (void)printf("jy_i==%d\n", jy_i);
                switch (jy_i)
                {
                case 0: add = JY_mb_ang; break;
                case 1: add = JY_mb_ang2; break;
                case 2: add = JY_mb_ang4; break;
                }
                if (add != NULL)
                {
                    g_hanzi_num = (uint16_t)(strlen((char *)add)); //拼音a的个数
                    memcpy(get_hanzi, add, g_hanzi_num);         //把找到的汉字放在缓冲区
                    *hh         = g_hanzi_num / 2;
                    (void)printf("HZ NUMBER=%d\n", g_hanzi_num / 2);
                    return true;
                }
            }
        }
        return false;
    }

    // 首字母为 'ㄦ'

    if (jy_ime_cmp == '-')
    {
        for (jy_i = 0; jy_i < sizeofER; jy_i++)                     // 16次
        {
            memcpy(jy_ime_temp, JY_index_er[jy_i], 5);              //   ai,an,ang,ao，ei,...   16个
            if (memcmp(&jy_ime_temp1[1], jy_ime_temp, 5) == 0)      // 输入的拼音和'a'的第2个韵母比较
            {
                (void)printf("jy_i==%d\n", jy_i);
                switch (jy_i)
                {
                case 0: add = JY_mb_er2; break;
                case 1: add = JY_mb_er3; break;
                case 2: add = JY_mb_er4; break;
                case 3: add = JY_mb_er5; break;
                }
                if (add != NULL)
                {
                    g_hanzi_num = (uint16_t)(strlen((char *)add)); //拼音a的个数
                    memcpy(get_hanzi, add, g_hanzi_num);         //把找到的汉字放在缓冲区
                    *hh         = g_hanzi_num / 2;
                    (void)printf("HZ NUMBER=%d\n", g_hanzi_num / 2);
                    return true;
                }
            }
        }
        return false;
    }

    // 首字母为 'ㄧ'

    if (jy_ime_cmp == 'u')
    {
        for (jy_i = 0; jy_i < sizeofY; jy_i++)                     // 16次
        {
            memcpy(jy_ime_temp, JY_index_y[jy_i], 5);              //   ai,an,ang,ao，ei,...   16个
            if (memcmp(&jy_ime_temp1[1], jy_ime_temp, 5) == 0)     // 输入的拼音和'a'的第2个韵母比较
            {
                (void)printf("jy_i==%d\n", jy_i);
                switch (jy_i)
                {
                case 0: add = JY_mb_yi; break;
                case 1: add = JY_mb_yi2; break;
                case 2: add = JY_mb_yi3; break;
                case 3: add = JY_mb_yi4; break;
                case 4: add = JY_mb_ya; break;
                case 5: add = JY_mb_ya2; break;
                case 6: add = JY_mb_ya3; break;
                case 7: add = JY_mb_ya4; break;
                case 8: add = JY_mb_ya5; break;
                case 9: add = JY_mb_yo; break;
                case 10: add = JY_mb_yo5; break;
                case 11: add = JY_mb_ye; break;
                case 12: add = JY_mb_ye2; break;
                case 13: add = JY_mb_ye3; break;
                case 14: add = JY_mb_ye4; break;
                case 15: add = JY_mb_yai2; break;
                case 16: add = JY_mb_yao; break;
                case 17: add = JY_mb_yao2; break;
                case 18: add = JY_mb_yao3; break;
                case 19: add = JY_mb_yao4; break;
                case 20: add = JY_mb_yao5; break;
                case 21: add = JY_mb_you; break;
                case 22: add = JY_mb_you2; break;
                case 23: add = JY_mb_you3; break;
                case 24: add = JY_mb_you4; break;
                case 25: add = JY_mb_yan; break;
                case 26: add = JY_mb_yan2; break;
                case 27: add = JY_mb_yan3; break;
                case 28: add = JY_mb_yan4; break;
                case 29: add = JY_mb_yin; break;
                case 30: add = JY_mb_yin2; break;
                case 31: add = JY_mb_yin3; break;
                case 32: add = JY_mb_yin4; break;
                case 33: add = JY_mb_yang; break;
                case 34: add = JY_mb_yang2; break;
                case 35: add = JY_mb_yang3; break;
                case 36: add = JY_mb_yang4; break;
                case 37: add = JY_mb_ying; break;
                case 38: add = JY_mb_ying2; break;
                case 39: add = JY_mb_ying3; break;
                case 40: add = JY_mb_ying4; break;
                }
                if (add != NULL)
                {
                    g_hanzi_num = (uint16_t)(strlen((char *)add)); //拼音a的个数
                    memcpy(get_hanzi, add, g_hanzi_num);         //把找到的汉字放在缓冲区
                    *hh         = g_hanzi_num / 2;
                    (void)printf("HZ NUMBER=%d\n", g_hanzi_num / 2);
                    return true;
                }
            }
        }
        return false;
    }

    // 首字母为 'ㄨ'

    if (jy_ime_cmp == 'j')
    {
        for (jy_i = 0; jy_i < sizeofW; jy_i++)                     // 16次
        {
            memcpy(jy_ime_temp, JY_index_w[jy_i], 5);              //   ai,an,ang,ao，ei,...   16个
            if (memcmp(&jy_ime_temp1[1], jy_ime_temp, 5) == 0)     // 输入的拼音和'a'的第2个韵母比较
            {
                (void)printf("jy_i==%d\n", jy_i);
                switch (jy_i)
                {
                case 0: add = JY_mb_wu; break;
                case 1: add = JY_mb_wu2; break;
                case 2: add = JY_mb_wu3; break;
                case 3: add = JY_mb_wu4; break;
                case 4: add = JY_mb_wa; break;
                case 5: add = JY_mb_wa2; break;
                case 6: add = JY_mb_wa3; break;
                case 7: add = JY_mb_wa4; break;
                case 8: add = JY_mb_wo; break;
                case 9: add = JY_mb_wo3; break;
                case 10: add = JY_mb_wo4; break;
                case 11: add = JY_mb_wai; break;
                case 12: add = JY_mb_wai4; break;
                case 13: add = JY_mb_wei; break;
                case 14: add = JY_mb_wei2; break;
                case 15: add = JY_mb_wei3; break;
                case 16: add = JY_mb_wei4; break;
                case 17: add = JY_mb_wan; break;
                case 18: add = JY_mb_wan2; break;
                case 19: add = JY_mb_wan3; break;
                case 20: add = JY_mb_wan4; break;
                case 21: add = JY_mb_wen; break;
                case 22: add = JY_mb_wen2; break;
                case 23: add = JY_mb_wen3; break;
                case 24: add = JY_mb_wen4; break;
                case 25: add = JY_mb_wang; break;
                case 26: add = JY_mb_wang2; break;
                case 27: add = JY_mb_wang3; break;
                case 28: add = JY_mb_wang4; break;
                case 29: add = JY_mb_weng; break;
                case 30: add = JY_mb_weng3; break;
                case 31: add = JY_mb_weng4; break;
                }
                if (add != NULL)
                {
                    g_hanzi_num = (uint16_t)(strlen((char *)add)); //拼音a的个数
                    memcpy(get_hanzi, add, g_hanzi_num);         //把找到的汉字放在缓冲区
                    *hh         = g_hanzi_num / 2;
                    (void)printf("HZ NUMBER=%d\n", g_hanzi_num / 2);
                    return true;
                }
            }
        }
        return false;
    }

    // 首字母为 'ㄩ'

    if (jy_ime_cmp == 'm')
    {
        for (jy_i = 0; jy_i < sizeofYU; jy_i++)                     // 16次
        {
            memcpy(jy_ime_temp, JY_index_yu[jy_i], 5);              //   ai,an,ang,ao，ei,...   16个
            if (memcmp(&jy_ime_temp1[1], jy_ime_temp, 5) == 0)     // 输入的拼音和'a'的第2个韵母比较
            {
                (void)printf("jy_i==%d\n", jy_i);
                switch (jy_i)
                {
                case 0: add = JY_mb_yu; break;
                case 1: add = JY_mb_yu2; break;
                case 2: add = JY_mb_yu3; break;
                case 3: add = JY_mb_yu4; break;
                case 4: add = JY_mb_yue; break;
                case 5: add = JY_mb_yue4; break;
                case 6: add = JY_mb_yuan; break;
                case 7: add = JY_mb_yuan2; break;
                case 8: add = JY_mb_yuan3; break;
                case 9: add = JY_mb_yuan4; break;
                case 10: add = JY_mb_yun; break;
                case 11: add = JY_mb_yun2; break;
                case 12: add = JY_mb_yun3; break;
                case 13: add = JY_mb_yun4; break;
                case 14: add = JY_mb_yong; break;
                case 15: add = JY_mb_yong2; break;
                case 16: add = JY_mb_yong3; break;
                case 17: add = JY_mb_yong4; break;
                }
                if (add != NULL)
                {
                    g_hanzi_num = (uint16_t)(strlen((char *)add)); //拼音a的个数
                    memcpy(get_hanzi, add, g_hanzi_num);         //把找到的汉字放在缓冲区
                    *hh         = g_hanzi_num / 2;
                    (void)printf("HZ NUMBER=%d\n", g_hanzi_num / 2);
                    return true;
                }
            }
        }
        return false;
    }


    return false;                      /*无果而终*/
}
