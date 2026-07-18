#include "Matrix.h"
#include <stdbool.h>
#define PI 3.14159265358979323846
typedef struct DotTable_ {
    uint16_t row;//Hn
    uint16_t col;//Wn
    uint16_t num;//2(x,y)
    float* data;
}DotTable;

typedef struct HUD_morph {
    float xbias;
    float ybias;
    float scale;
    float angle;
    float xscale;
    float yscale;
    //float tscale;
    //float bscale;
    //float lscale;
    //float rscale;
}HUD_morph;

typedef struct hud_header {
    char tagH[3];
    uint16_t LcdW;
    uint16_t LcdH;
    uint16_t DotnumW;
    uint16_t DotnumH;
    uint16_t BlockW;
    uint16_t BlockH;
    uint16_t Empty_x_left;
    uint16_t Empty_x_right;
    uint16_t Empty_y_top;
    uint16_t Empty_y_btm;
    uint16_t Regression;
    uint16_t Target_x_left;
    uint16_t Target_x_right;
    uint16_t Target_y_top;
    uint16_t Target_y_btm;
    char taginfo[4];
    uint8_t Tablebias;
    uint8_t Tabletype;
    uint8_t Rectype;
    uint8_t Fitting;
    uint8_t Alignment;
    bool Table_flip;
    bool Table_mirro;
    char tagMorph[5];
    HUD_morph m;
    char tagData[4];
} hud_header;

typedef struct HUD_format {
    hud_header head;
    DotTable tab;
}HUD_format;

typedef enum {
    FLIP=0,
    MIRRO=1,
} TableTrans;

int table_to_array(const char* filepath,DotTable *table,uint32_t ext);
void array_to_table(const char* filepath,DotTable *table);
void array_to_bin(const char* filepath, DotTable* table);
int gen_mesh_array(DotTable *table,uint32_t lcdH,uint32_t lcdW,uint32_t blockH,uint32_t blockW);

Matrix* linear_solve(float* arrayxy, int N);
Matrix* Linear_fitting(MATRIX_TYPE* dot, Matrix* slove);
Matrix* getlineCross(Matrix* s0, Matrix* s1);
Matrix* quadratic_solve(float* arrayxy, int N);
Matrix* Quadratic_fitting(MATRIX_TYPE* dot, Matrix* slove);
Matrix* getQuadraticLineCross(Matrix* s0, Matrix* s1, MATRIX_TYPE* dotp);
Matrix* getQuadraticCross(Matrix* s0, Matrix* s1, MATRIX_TYPE* dotp);

Matrix* polynomial_solve(float* arrayxy, int N, unsigned int np);
Matrix* Polynomial_fitting(MATRIX_TYPE* dot, Matrix* slove);
Matrix* getPolynomialCross(Matrix* s0, Matrix* s1, MATRIX_TYPE* dotp);
void TableInit(DotTable *table,int r,int c,int n);
void TableDestroy(DotTable *table);
void TableTranspose(DotTable *table,int ext);
void TableTransfer(DotTable *table,float parm,TableTrans type);
void TableTransfer2(DotTable* table, float parm, TableTrans type);
float *TabAddr(DotTable *TAB,int r,int c,int n);
float TabVal(DotTable *TAB,int r,int c,int n);
void TableCopy(DotTable* dist, DotTable* src);

void HUDImgTransfer(float* array, hud_header* h);
void HUDalignment(float* shiftarr, hud_header* h);
void HUDpreProcess(float* shiftarr, hud_header* h);
void HUDFittedProcess(DotTable* TAB, hud_header* h);
void tableExternVirtual(DotTable* WT, DotTable* ST, uint32_t lcdH, uint32_t lcdW);

Matrix *getPerspectiveM(float* src,float* dst);
int isInRec(float *recP,float *point);

void genPerspectivePlot(DotTable *stdRecT,DotTable *warpRecT,DotTable *inputT,DotTable *distT);
void MdotApply(Matrix *M,float *dot);

void tableMathAdd(DotTable* T1, DotTable* T2);
void tableMathSub(DotTable* T1, DotTable* T2);
void tableMathShift(DotTable* T1, float x, float y);
void tableMathRotate(DotTable* T1, float angle);
void tableMathmulfactor(DotTable* T1, float factor);
float resizefactor(float Htar, float Wtar, float Hsrc, float Wsrc);

float* getTableRec(DotTable* T);
float* getMAXinterRec(DotTable* T);
float* getMINexterRec(DotTable* T);
//void tiltangle(float* Tab);


/*HUD fitting method*/
void HUDFitting1(DotTable* TAB, hud_header* h);//Q:w
void HUDFitting2(DotTable* TAB, hud_header* h);//Q :W Q:H cross 
void HUDFitting3(DotTable* TAB, hud_header* h);//Q :W L:H(separate)
void HUDFitting4(DotTable* TAB, hud_header* h);//Q :Q L:H cross
void HUDFitting5(DotTable* TAB, hud_header* h);//Q :W Q:H (separate)
void HUDFittingP(DotTable* TAB, hud_header* h, int Phn, int Pvn); //user define :polynomial horizon curve :Phn , polynomial vertical curve :Pvn ,

int HudFileSave(char* path, hud_header* h, DotTable* tab);
HUD_format* HudFileLoad(char* path);
void Hudrelease(HUD_format* h);