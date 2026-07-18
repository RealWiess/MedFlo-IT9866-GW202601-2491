#ifndef _ITU_UTILTY_H
#define _ITU_UTILTY_H

#ifdef __cplusplus
extern "C" {
#endif

static inline ITUListBox* initITUListBox(const char *name) {
    ITUListBox* listbox = (ITUListBox*)ituSceneFindWidget(&theScene, name);
    assert(listbox);
    return listbox;
}

static void ListBoxOnLoadPage(ITUListBox* listbox, int pageIndex)
{
    ITCTree* node;
    int i, count;
    assert(listbox);

    count = itcTreeGetChildCount(listbox);

    node = ((ITCTree*)listbox)->child;

    for (i = 0; i < count; i++)
    {
        char buf[33];
        ITUScrollText* scrolltext = (ITUScrollText*) node;

        sprintf(buf, "%d", ((pageIndex - 1) * count + i) % count);
        ituTextSetString(scrolltext, buf);

        node = node->sibling;
    }
    listbox->pageIndex = pageIndex;
    listbox->itemCount = count;
}

static void ListOnSelection(ITUListBox* listbox, ITUScrollText* item, bool confirm)
{
    assert(listbox);

    printf("%s selected. confirm: %d\n", item->text.string, confirm);
}

static inline ITUIconListBox* initITUIconListBox(const char *name) {
    ITUIconListBox* iconlistbox = (ITUIconListBox*)ituSceneFindWidget(&theScene, name);
    assert(iconlistbox);

    ituListBoxSetOnLoadPage(iconlistbox, ListBoxOnLoadPage);

    iconlistbox->listbox.pageIndex = 1; // on first page
    iconlistbox->listbox.pageCount = 1; // total 1 page

    // init first page data
    ListBoxOnLoadPage(iconlistbox, 1);
    ituWidgetUpdate(iconlistbox, ITU_EVENT_LAYOUT, 0, 0, 0);

    ituListBoxSelect(iconlistbox, 0);

    return iconlistbox;
}

static inline ITUScrollIconListBox* initITUScrollIconListBox(const char *name) {
    ITUScrollIconListBox* scrolliconlistbox = (ITUScrollIconListBox*)ituSceneFindWidget(&theScene, name);
    assert(scrolliconlistbox);
    return scrolliconlistbox;
}

static inline ITUButton* initITUButton(const char *name) {
    ITUButton* button = (ITUButton*)ituSceneFindWidget(&theScene, name);
    assert(button);
    return button;
}

static inline ITURadioBox* initITURadioBox(const char *name) {
    ITURadioBox* radioBox = (ITURadioBox*)ituSceneFindWidget(&theScene, name);
    assert(radioBox);
    return radioBox;
}

static inline ITUSprite* initITUSprite(const char *name) {
    ITUSprite* sprite = (ITUSprite*)ituSceneFindWidget(&theScene, name);
    assert(sprite);
    return sprite;
}

static inline ITUIcon* initITUIcon(const char *name) {
    ITUIcon* icon = (ITUIcon*)ituSceneFindWidget(&theScene, name);
    assert(icon);
    return icon;
}

static inline ITUText* initITUText(const char *name) {
    ITUText* text = (ITUText*)ituSceneFindWidget(&theScene, name);
    assert(text);
    return text;
}

static inline ITUCircleProgressBar* initITUCircleProgressBar(const char *name) {
    ITUCircleProgressBar* circleProgressBar = (ITUCircleProgressBar*)ituSceneFindWidget(&theScene, name);
    assert(circleProgressBar);
    return circleProgressBar;
}

static inline ITUMeter* initITUMeter(const char *name) {
    ITUMeter* meter = (ITUMeter*)ituSceneFindWidget(&theScene, name);
    assert(meter);
    return meter;
}

static inline ITUContainer* initITUContainer(const char *name) {
    ITUContainer* container = (ITUContainer*)ituSceneFindWidget(&theScene, name);
    assert(container);
    return container;
}

static inline ITUBackground* initITUBackground(const char *name) {
    ITUBackground* background = (ITUBackground*)ituSceneFindWidget(&theScene, name);
    assert(background);
    return background;
}

static inline ITULayer* initITULayer(const char *name) {
    ITULayer* layer = (ITULayer*)ituSceneFindWidget(&theScene, name);
    assert(layer);
    return layer;
}

static inline ITUSurface* swapITUSurface(ITUSurface **a, ITUSurface **b)
{
    ITUSurface* tmp = *a;
    *a = *b;
    *b = tmp;
}

static inline void ITULinkSurfaceIconListboxWithIcon(ITUIconListBox *iconListbox, ITUIcon *listbox
                                                     ,int pos)
{
    iconListbox->staticSurfArray[pos] = listbox->staticSurf;
}

static inline void initITUIconSrc(ITUIcon** iconArr, uint8_t* name, uint8_t num)
{
    int i;
    char temp[64];

    for (i = 0; i < num; i++)
    {
        memset(temp, 0, sizeof(temp));
        sprintf(temp, "%s%d", name, i);
        iconArr[i] = initITUIcon(temp);
    }
}

#ifdef __cplusplus
}
#endif
#endif
