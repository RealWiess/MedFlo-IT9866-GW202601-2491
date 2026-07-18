#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include "openrtos/FreeRTOS.h"
#include "ite/itp.h"
#include "dewarp.h"
#include "HUDTool.h"
#include "Config.ini"

#define LCDCOORDINATE 1
#define SAVEHUDTABLE 0
DotTable gdewarpmeshTable={};

float *table_read(const char* filepath){
    int Hn=DOT_HNUM;
    int Wn=DOT_WNUM;
    
    FILE *f=fopen(filepath,"rb");
    
    if(f!=NULL)
    {
        float *array=malloc(sizeof(float)*Hn*Wn*2);
        if (array != NULL) {
            memset(array,0,sizeof(float)*Hn*Wn*2);
            int i=0;
            while(i<Hn*Wn*2){
                (void)fscanf(f, "%f", &array[i]);
                i++;
            }
            fclose(f);
            return array;
        }else{
            printf("%s not exit\n",filepath);
        }
    }
    return NULL;
}

static int gen_ext_table(DotTable *Table,float *array,int ext){
    int Hn=DOT_HNUM;
    int Wn=DOT_WNUM;    
    
    for(int i=0;i<Hn;i++)
    {
        for(int j=0;j<Wn;j++)
        {
            *TabAddr(Table,i+ext,j+ext,0)=array[i*Wn*2+j*2+0];
            *TabAddr(Table,i+ext,j+ext,1)=array[i*Wn*2+j*2+1];  
        }
    }
    return 1;
}

static void gen_stdDot_array(float* array) {
    int Hn = DOT_HNUM;
    int Wn = DOT_WNUM;
    float ori[2] = { EMPTY_X_LEFT ,EMPTY_Y_TOP };
    float Xdist = (float)(LCDW - EMPTY_X_LEFT - EMPTY_X_RIGHT) / (Wn - 1);
    float Ydist = (float)(LCDH - EMPTY_Y_TOP - EMPTY_Y_BOTM) / (Hn - 1);
    for (int i = 0; i < Hn; i++) {
        for (int j = 0; j < Wn; j++)
        {
            (*array++) = j * Xdist + EMPTY_X_LEFT;
            (*array++) = i * Ydist + EMPTY_Y_TOP;
        }
    }
}

static int HUDPosttest(DotTable* Table) {
    int Hn = Table->row;
    int Wn = Table->col;
    int result = 0;
    for (int i = 0;i < Hn;i++)
    {
        for (int j = 0;j < Wn;j++) {
            int err = 0;
            float x = TabVal(Table, i, j, 0);
            float y = TabVal(Table, i, j, 1);

            if (abs(x) >= BLOCK_WIDTH * 5) {
                err = 1;
            }
            if (abs(y) >= BLOCK_HEIGHT * 5) {
                err = 1;
            }
            if (err) {
                printf("ITE HUD BlockH/W too small abs(%f,%f)>(%d*5,%d*5)\n", x, y, BLOCK_WIDTH, BLOCK_HEIGHT);
                result = 1;
            }
        }
    }
    return result;
}

static void table_release(DotTable* std, DotTable* warp, DotTable* mesh, DotTable* dewarmesh) {
    TableDestroy(std);
    TableDestroy(warp);
    TableDestroy(mesh);
    TableDestroy(dewarmesh);
}

void HUDTableFLIP(void){
    TABLE_FLIP    =1;
    TABLE_MIRRO   =0;
}
void HUDTableMIRRO(void){
    TABLE_FLIP    =0;
    TABLE_MIRRO   =1;
}
void HUDTableROTATE180(void){
    TABLE_FLIP    =1;
    TABLE_MIRRO   =1;
}

void HUDUpdateBlock(int bwidth,int bheigh){
    BLOCK_WIDTH=bwidth;
    BLOCK_HEIGHT=bheigh;
}

void HUDUpdateMesh(float dx,float dy,float angle,float scale,float xscale ,float yscale){
    xshiftMesh=dx;
    yshiftMesh=dy;
    angleMesh =angle;
    scaleMesh =scale;
    xscaleMesh=xscale;
    yscaleMesh=yscale;
}

void HUDUpdateBlankFit(int blank,int fitting){
    MAX_REC_BLANK = blank;
    FITTING = fitting;
}

int HUDUpdateWarp(const char *warpTable,Table *dewarpmesh){
    int err=0;
    if ((access(warpTable, 0) != -1)){
        float *arraywarp=table_read(warpTable);
        if(arraywarp!=NULL){
            err =HUDdewarpTable(NULL,arraywarp,dewarpmesh);
            free(arraywarp);   
        }
    }
    return err;
}

static void HUDheaderInit(hud_header* h) {
    (void)strncpy(&h->tagH[0], "HUD", 3);
    h->LcdW = LCDW;
    h->LcdH = LCDH;
    h->DotnumW = DOT_WNUM;
    h->DotnumH = DOT_HNUM;
    h->BlockW = BLOCK_WIDTH;
    h->BlockH = BLOCK_HEIGHT;
    h->Empty_x_left = EMPTY_X_LEFT;
    h->Empty_x_right = EMPTY_X_RIGHT;
    h->Empty_y_top = EMPTY_Y_TOP;
    h->Empty_y_btm = EMPTY_Y_BOTM;
    h->Regression = REGRESSION;
    h->Target_x_left = TARGET_X_LEFT;
    h->Target_x_right = TARGET_X_RIGHT;
    h->Target_y_top = TARGET_Y_TOP;
    h->Target_y_btm = TARGET_Y_BOTM;
    (void)strncpy(&h->taginfo[0], "case", 4);
    h->Tabletype = 0;
	h->Tablebias = 0;
    h->Rectype = MAX_REC_BLANK;
    h->Fitting = FITTING;
    h->Alignment = ALIGNMENT;
    h->Table_flip = TABLE_FLIP;
    h->Table_mirro = TABLE_MIRRO;
    (void)strncpy(&h->tagMorph[0], "morph", 5);
    h->m.xbias = xshiftMesh;
    h->m.ybias = yshiftMesh;
    h->m.scale = scaleMesh;
    h->m.angle = angleMesh ;
    h->m.xscale = xscaleMesh;
    h->m.yscale = yscaleMesh;
    (void)strncpy(&h->tagData[0], "data", 4);

}

int HUDdewarpTable(float *stdarr,float *shiftarr,Table *dewarpmesh)
{
    bool stdautogen = false;
    hud_header h;
    HUDheaderInit(&h);
    int result=GENSUCCESS;
    uint32_t eHnum = h.DotnumH+2;
    uint32_t eWnum = h.DotnumW+2;
    uint32_t bWnum = h.LcdW/h.BlockW+1;
    uint32_t bHnum = h.LcdH/h.BlockH+1;

    printf("LCD(H,W)  = (%d,%d)\n", h.LcdH, h.LcdW);
    printf("dot(h,w)  = (%d,%d)\n", h.DotnumH, h.DotnumW);
    printf("block(h,w)= (%d,%d)\n", h.BlockH, h.BlockW);
    printf("boundary(L,R,T,B) = (%d,%d,%d,%d)\n", h.Empty_x_left, h.Empty_x_right, h.Empty_y_top, h.Empty_y_btm);
    printf("table transform(mirro,flip) =(%d,%d)\n", h.Table_mirro, h.Table_flip);
    printf("MAX_REC_BLANK = %d FITTING = %d ALIGNMENT = %d regression = %d\n", h.Rectype, h.Fitting,h.Alignment, h.Regression);
    printf("scale:%f angle=%f ,shift(%f,%f) xyscale(%f ,%f) \n", scaleMesh,angleMesh, xshiftMesh, yshiftMesh,xscaleMesh,yscaleMesh);

    if (stdarr == NULL) {
        stdarr = (float*)malloc(sizeof(float) * h.DotnumH * h.DotnumW * 2);
        if (stdarr != NULL) {
            gen_stdDot_array(stdarr);
            printf("Auto gen stddot array\n");
        }
        stdautogen = true;
    }
    if (!LCDCOORDINATE) {
        for (int i = 0;i < h.DotnumH;i++)
        {
            for (int j = 0;j < h.DotnumW;j++)
            {
                stdarr[i * h.DotnumW * 2 + j * 2 + 0]+= h.Empty_x_left;
                stdarr[i * h.DotnumW * 2 + j * 2 + 1]+= h.Empty_y_top;
                shiftarr[i * h.DotnumW * 2 + j * 2 + 0] = stdarr[i * h.DotnumW * 2 + j * 2 + 0] - shiftarr[i * h.DotnumW * 2 + j * 2 + 0];
                shiftarr[i * h.DotnumW * 2 + j * 2 + 1] = stdarr[i * h.DotnumW * 2 + j * 2 + 1] - shiftarr[i * h.DotnumW * 2 + j * 2 + 1];

            }
        }
    }
    if (h.Table_flip || h.Table_mirro) {
        HUDImgTransfer(shiftarr, &h);
    }
    if (h.Alignment!=0){
        HUDalignment(shiftarr,&h);
    }
    if (h.Rectype != 0) {
        HUDpreProcess(shiftarr, &h);
    }
    
#if 1
    DotTable extstdTable;
    DotTable extwarpTable;
    DotTable stdmeshTable;
    DotTable dewarpmeshTable;
    
    if(stdarr == NULL)
        return STDARRAYerr;//input NULL
    if(shiftarr == NULL)
        return WARPARRYerr;//input NULL
        
    
    TableInit(&extstdTable,eHnum,eWnum,2);
    TableInit(&extwarpTable,eHnum,eWnum,2);
    TableInit(&stdmeshTable,bHnum,bWnum,2);
    TableInit(&dewarpmeshTable,bHnum,bWnum,2);
    

    gen_ext_table(&extstdTable,stdarr,1);
    gen_ext_table(&extwarpTable,shiftarr,1);
    gen_mesh_array(&stdmeshTable,h.LcdH,h.LcdW,h.BlockH,h.BlockW);

    tableExternVirtual(&extwarpTable, &extstdTable, h.LcdH, h.LcdW);
    
    genPerspectivePlot(&extstdTable,&extwarpTable,&stdmeshTable,&dewarpmeshTable);

    if (gdewarpmeshTable.data != NULL) {
        TableCopy(&gdewarpmeshTable, &dewarpmeshTable);//copy to global Table for regression
    }

    #if 1
    for(int i=0;i<bHnum;i++)
    {
        for(int j=0;j<bWnum;j++)
        {
            *TabAddr(&dewarpmeshTable,i,j,0)-=TabVal(&stdmeshTable,i,j,0);
            *TabAddr(&dewarpmeshTable,i,j,1)-=TabVal(&stdmeshTable,i,j,1);
        }
    }

    {//xy Transpse for HUD table
        TableTranspose(&stdmeshTable,0);//HUDstd mesh table 
        TableTranspose(&dewarpmeshTable,0);//HUD dewarpdist table
        #if SAVEHUDTABLE
        array_to_table(dewarpmeshdistPath,&dewarpmeshTable);
        array_to_table(standardTPath,&stdmeshTable);        
        #endif
        if(dewarpmesh!=NULL){
            int size =sizeof(float)*bHnum*bWnum*2;
            if(dewarpmeshTable.data !=NULL){
                dewarpmesh->hn=bHnum;
                dewarpmesh->wn=bWnum;
                dewarpmesh->nn=2;
                if (dewarpmesh->data == NULL)
                    dewarpmesh->data = malloc(size);
                if(dewarpmesh->data!=NULL)
                    memcpy(dewarpmesh->data,dewarpmeshTable.data,size);
            }
        }
    }

    //array_to_table(dewarpmeshPath,&dewarpmeshTable);
#endif
    if (stdautogen) {//free autogen stdarr
        free(stdarr);
    }
    if (HUDPosttest(&dewarpmeshTable) == 1) {
        table_release(&extstdTable, &extwarpTable, &stdmeshTable, &dewarpmeshTable);
        return DESTINATIONerr;
    }
    else {
        table_release(&extstdTable, &extwarpTable, &stdmeshTable, &dewarpmeshTable);
        return GENSUCCESS;
    }
#endif
}

int HUDRegression(float* stdarr, float* warparr, Table* warpmesh) {
    uint32_t bWnum = LCDW / BLOCK_WIDTH + 1;
    uint32_t bHnum = LCDH / BLOCK_HEIGHT + 1;
    int bitsize = sizeof(float) * DOT_WNUM * DOT_HNUM * 2;
    float xshift = xshiftMesh;
    float yshift = yshiftMesh;
    float scalef = scaleMesh;
    int count = REGRESSION;
    float* arraywarp = NULL;
    float * arraystd = NULL;
    int err;

    if (warparr == NULL)
        return 0;
    else
        arraywarp = (float*)malloc(bitsize);

    if (stdarr == NULL)
        arraystd = NULL;
    else
        arraystd = (float*)malloc(bitsize);

    TableInit(&gdewarpmeshTable, bHnum, bWnum, 2);

    do  {
        printf("\n\n[HUDRegression] %d\n", count);
        if(arraystd) memcpy((float*)arraystd, (float*)stdarr, bitsize);
        if(arraywarp) memcpy((float*)arraywarp, (float*)warparr, bitsize);
        err = HUDdewarpTable((float*)arraystd, (float*)arraywarp, warpmesh);

        if (1) {
            float Cent[2] = {0}; //external rec center
            float tCent[2] = {0}; //Target rec center
            float vH;
            float vW;
            float tH;//target region
            float tW;//target region
            float* pRec = getMINexterRec(&gdewarpmeshTable);
            float Rec[4][2];
            float factor;
            memcpy(Rec, pRec, 8 * sizeof(float));
            if (pRec) free(pRec);
            vH = (Rec[2][1] - Rec[0][1]);
            vW = (Rec[2][0] - Rec[0][0]);
            Cent[0] = (float)(Rec[2][0] + Rec[0][0]) / 2;
            Cent[1] = (float)(Rec[2][1] + Rec[0][1]) / 2;
            tW = LCDW - (TARGET_X_LEFT + TARGET_X_RIGHT);
            tH = LCDH - (TARGET_Y_TOP + TARGET_Y_BOTM);
            if ((float)(vW / vH) > ((float)tW / (float)tH))
                factor = (float)vW / tW;
            else
                factor = (float)vH / tH;
            tCent[0] = (Cent[0] - (TARGET_X_LEFT + tW / 2)) + xshiftMesh;
            tCent[1] = (Cent[1] - (TARGET_Y_TOP + tH / 2)) + yshiftMesh;
            factor *= scaleMesh;
            xshiftMesh = rint(tCent[0]);
            yshiftMesh = rint(tCent[1]);
            scaleMesh = (float)((int)(factor*10000+0.5))/10000;
        }

        if ((fabs(xshift - xshiftMesh) < 1.0) &&
            (fabs(yshift - yshiftMesh) < 1.0) &&
            ((fabs(scalef - scaleMesh)) <= 0.00001)) {
            break;
        }
        else {
            xshift = xshiftMesh;
            yshift = yshiftMesh;
            scalef = scaleMesh;
        }
        count--;

    } while (count >= 0);
    TableDestroy(&gdewarpmeshTable);
    if (arraystd) free(arraystd);
    if (arraywarp) free(arraywarp);

    printf("\n[HUDRegression] suggest value\n");
    printf("T:xshiftmesh:%f\n", xshiftMesh);
    printf("T:yshiftmesh:%f\n", yshiftMesh);
    printf("T:scaleMesh:%f\n", scaleMesh);
    return err;
}