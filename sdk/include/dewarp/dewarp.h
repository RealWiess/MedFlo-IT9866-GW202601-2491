typedef struct Table_ {
	int hn;//Hn
	int wn;//Wn
	int nn;//2(x,y)
	float* data;
}Table;

enum {
  GENSUCCESS=0, //HUDtable success
  STDARRAYerr,  //input standard dot array NULL
  WARPARRYerr,  //input warp dot array NULL
  STDCOORDINATEerr, //standard dot coordinate over LCDW/LCDH
  WARPCOORDINATEerr,//warp dot coordinate over LCDW/LCDH
	BLOCKSIZEerr,// LCDW/blockw or LCDH/blockh not interger
  DESTINATIONerr,//dewarp destination over (BLOCK_WIDTH*5) or (BLOCK_HEIGHT*5)
};

/*table flit,mirro,rotate180*/
void HUDTableFLIP(void);
void HUDTableMIRRO(void);
void HUDTableROTATE180(void);

/*update block*/
void HUDUpdateBlock(int bwidth,int bheigh);
/*update HUD transform*/
void HUDUpdateMesh(float dx,float dy,float angle,float scale,float xscale ,float yscale);
/*update parm*/
void HUDUpdateBlankFit(int blank,int fitting);
/*update warpTable*/
int HUDUpdateWarp(const char *warpTable,Table *dewarpmesh);

/*HUD dewarptable generate*/
int HUDdewarpTable(float *stdarr,float *shiftarr,Table *dewarpmesh);
/*HUD dewarptable generate regression*/
int HUDRegression(float* stdarr, float* warparr, Table* warpmesh);

/*read txt*/
float *table_read(const char* filepath);