#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "Windows.h"
#include "math.h"


//几个全局变量，存放读入图像的位图数据、宽、高、颜色表及每像素所占位数(比特) 
//此处定义全局变量主要为了后面的图像数据访问及图像存储作准备
unsigned char *BmpBuf;//读入图像数据的指针
int bmpWidth;//图像的宽
int bmpHeight;//图像的高
int imgSpace;//图像所需空间
RGBQUAD *pColorTable;//颜色表指针
int biBitCount;//图像类型
char str[100];//文件名称 
int Num[300];//各灰度值出现的次数 
float Feq[300];//各灰度值出现的频率 
unsigned char *lpBuf;//指向图像像素的指针
unsigned char *m_pDib;//存放打开文件的DIB


int NodeNum;	//Huffman树总节点个数
int NodeStart;	//Huffman树起始节点
struct Node{		//Huffman树节点
	int gray;		//记录叶子节点的灰度值（非叶子节点为 -1）
	int lson,rson;	//节点的左右儿子（若没有则为 -1）
	int num;		//节点的数值（编码依据）
	int use;		//记录节点是否被用过(用过为1，没用过为0)
}node[600];

char CodeTmp[300];
char CodeStr[300][300];	//记录编码值
int CodeLen[300];		//编码长度
bool ImgInf[8000000];	//图像信息
int InfLen;				//图像信息长度


//打开图像文件的函数，0为失败,1为成功
bool readFile(char *filePath)
{
	//二进制读方式打开指定的图像文件
	FILE *fp=fopen(filePath,"rb");
	if(fp==0)
	{ 
		printf("未找到指定文件！\n");
		return 0;
	}


	//跳过位图文件头结构BITMAPFILEHEADER
	fseek(fp, sizeof(BITMAPFILEHEADER),0);


	//读入信息头
	BITMAPINFOHEADER head;  
	fread(&head, sizeof(BITMAPINFOHEADER), 1,fp); 

	//获取图像宽、高、每像素所占位数等信息
	bmpWidth = head.biWidth;
	bmpHeight = head.biHeight;
	biBitCount = head.biBitCount;

	//定义变量，计算图像每行像素所占的字节数（必须是4的倍数）
	int lineByte=(bmpWidth * biBitCount/8+3)/4*4;

	//灰度图像有颜色表，且颜色表表项为256
	if(biBitCount==8)
	{
		//申请颜色表所需要的空间，读颜色表进内存
		pColorTable=new RGBQUAD[256];
		fread(pColorTable,sizeof(RGBQUAD),256,fp);
	}

	//申请位图数据所需要的空间，读位图数据进内存
	BmpBuf=new unsigned char[lineByte * bmpHeight];
	fread(BmpBuf,1,lineByte * bmpHeight,fp);

	//关闭文件
	fclose(fp);

	return 1;
}
/*********************

Huffman编码

*********************/
//Huffman编码初始化
void HuffmanCodeInit()
{
	int i;
	for(i = 0;i <256;i ++)//灰度值记录清零
		Num[i] = 0;
	//初始化哈夫曼树
	for(i = 0;i < 600;i ++)
	{
		node[i].gray = -1;
		node[i].lson  = node[i].rson = -1;
		node[i].num   = -1;
		node[i].use  = 0;
	}
	NodeNum = 0;
}
//深度搜索遍历Huffman树
void dsearch(int pos,int len)
{
	//遍历左儿子
	if(node[pos].lson != -1)
	{
		CodeTmp[len] = '1';
		dsearch(node[pos].lson,len + 1);
	}
	else
	{
		if(node[pos].gray != -1)
		{
			CodeLen[node[pos].gray] = len;
			CodeTmp[len] = '\0';
			strcpy(CodeStr[node[pos].gray],CodeTmp);
		}
	}

	//遍历右儿子
	if(node[pos].lson != -1)
	{
		CodeTmp[len] = '0';
		dsearch(node[pos].rson,len + 1);
	}
	else{
		if(node[pos].gray != -1)
		{
			CodeLen[node[pos].gray] = len;
			CodeTmp[len] = '\0';
			strcpy(CodeStr[node[pos].gray],CodeTmp);
		}
	}
}


//寻找值最小的节点
int MinNode()
{
	int i,j = -1;
	for(i = 0;i < NodeNum;i ++)
		if(!node[i].use)
			if(j == -1 || node[i].num < node[j].num)
				j = i;
	if(j != -1)
	{
		NodeStart = j;
		node[j].use = 1;
	}
	return j;
}

//二进制转十进制
int Change2to10(int pos){
	int i,j,two = 1;
	j = 0;
	for(i = pos + 7;i >= pos;i --){
		j += two * ImgInf[i];
		two *= 2;
	}
	return j;
}

//Huffman编码
void HuffmanCode()
{
	int i,j,k,a,b;

	for(i = 0;i < 256;i ++)
	{//创建初始节点
		Feq[i] = (float)Num[i] / (float)(bmpHeight * bmpWidth);//计算灰度值频率
		//Num[i] = (float)Num[i] / (float)(bmpHeight * bmpWidth);//计算灰度值频率
		if(Num[i] > 0)
		{
			node[NodeNum].gray = i;
			node[NodeNum].num = Num[i];
			node[NodeNum].lson = node[NodeNum].rson = -1;	//叶子节点无左右儿子
			NodeNum ++;
		}
	}

	while(1)
	{	//找到两个值最小的节点，合并成为新的节点
		a = MinNode();
		if(a == -1)
			break;
		b = MinNode();
		if(b == -1)
			break;

		//构建新节点
		node[NodeNum].gray = -1;
		node[NodeNum].num = node[a].num + node[b].num;
		node[NodeNum].lson = a;
		node[NodeNum].rson = b;
		NodeNum ++;
	}

	//根据建好的Huffman树编码进行深度搜索
	dsearch(NodeStart,0);


	//记录图像信息
	InfLen = 0;
	int lineByte=(bmpWidth * biBitCount/8+3)/4*4;
	for(i = 0;i < bmpHeight;i ++)
		for(j = 0;j < bmpWidth;j ++)
		{
			lpBuf = (unsigned char *)BmpBuf + lineByte * i + j;
			for(k = 0;k < CodeLen[*(lpBuf)];k ++)
			{
				ImgInf[InfLen ++] = (int)(CodeStr[*(lpBuf)][k] - '0'); 
			}
		}

	//再编码数据
	j = 0;
	for(i = 0;i < InfLen;)
	{
		*(BmpBuf + j) = Change2to10(i);
		i += 8;
		j ++;
	}
}

//保存压缩后的文件
bool saveBmp(char *bmpName, unsigned char *imgBuf, int width, int height, 
			 int biBitCount, RGBQUAD *pColorTable)
{
	//如果位图数据指针为0,则没有数据传入,函数返回
	if(!imgBuf)
		return 0;

	//颜色表大小,以字节为单位,灰度图像颜色表为1024字节,彩色图像颜色表大小为0
	int colorTablesize=0;
	if(biBitCount==8)
		colorTablesize=1024;

	//待存储图像数据每行字节数为4的倍数
	int lineByte=(width * biBitCount/8+3)/4*4;

	//以二进制写的方式打开文件
	FILE *fp=fopen(bmpName,"wb");
	if(fp==0) return 0;

	//申请位图文件头结构变量，填写文件头信息
	BITMAPFILEHEADER fileHead;
	fileHead.bfType = 0x4D42;//bmp类型

	//bfSize是图像文件4个组成部分之和
	fileHead.bfSize= sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER)
		+ colorTablesize + lineByte*height;
	fileHead.bfReserved1 = 0;
	fileHead.bfReserved2 = 0;

	//bfOffBits是图像文件前三个部分所需空间之和
	fileHead.bfOffBits=54+colorTablesize;

	//写文件头进文件
	fwrite(&fileHead, sizeof(BITMAPFILEHEADER),1, fp);

	//申请位图信息头结构变量，填写信息头信息
	BITMAPINFOHEADER head; 
	head.biBitCount=biBitCount;
	head.biClrImportant=0;
	head.biClrUsed=0;
	head.biCompression=0;
	head.biHeight=height;
	head.biPlanes=1;
	head.biSize=40;
	head.biSizeImage=lineByte*height;
	head.biWidth=width;
	head.biXPelsPerMeter=0;
	head.biYPelsPerMeter=0;
	//写位图信息头进内存
	fwrite(&head, sizeof(BITMAPINFOHEADER),1, fp);

	//如果灰度图像,有颜色表,写入文件 
	if(biBitCount==8)
		fwrite(pColorTable, sizeof(RGBQUAD),256, fp);

	//写位图数据进文件
	//fwrite(imgBuf, height*lineByte, 1, fp);
	fwrite(imgBuf, InfLen / 8, 1, fp);


	//关闭文件
	fclose(fp);

	return 1;
}

//输出Huffman编码树码表的信息
int saveTreeInfo(char *writePath,int lineByte){
	int i;

	FILE *ftxt;
	ftxt = fopen(writePath,"w");
	//fprintf(fout,"%d %d %d\n",NodeStart,NodeNum,InfLen);//输出起始节点、节点总数、图像所占空间
	for(i = 0;i < 256;i ++)
	if(Num[i] > 0)
	{		//输出Huffman树
	       
           fprintf(ftxt,"灰度值：%3d  频率: %f  码长: %2d  编码: %s\n",i,Feq[i],CodeLen[i],CodeStr[i]);      
	}



	fclose(ftxt);

	return 0;
}

//保存Huffman编码信息 
int saveInfo(char *writePath,int lineByte){
	int i,j,k;

	FILE *fout;
	fout = fopen(writePath,"w");
	fprintf(fout,"%d %d %d\n",NodeStart,NodeNum,InfLen);//输出起始节点、节点总数、图像所占空间
	for(i = 0;i <  NodeNum;i ++){		//输出Huffman树
            fprintf(fout,"%d %d %d\n",node[i].gray,node[i].lson,node[i].rson);    
	}



	fclose(fout);

	return 0;
}

/*********************

Huffman编码图像解码

*********************/
//读入编码图像
bool readHuffman(char *Name)
{
	int i,j;
	char NameStr[100];
	//读取Huffman编码信息和编码树
	strcpy(NameStr,Name);
	strcat(NameStr,".bpt");
	FILE *fin = fopen(NameStr,"r");
	if(fin == 0){
		printf("未找到指定文件！\n");
		return 0;
	}
	fscanf(fin,"%d %d %d",&NodeStart,&NodeNum,&InfLen);
	//printf("%d %d %d\n",NodeStart,NodeNum,InfLen);
	for(i = 0;i < NodeNum;i ++){
		fscanf(fin,"%d %d %d",&node[i].gray,&node[i].lson,&node[i].rson);
		//printf("%d %d %d\n",node[i].color,node[i].lson,node[i].rson);
	}

	/********
	二进制读方式打开指定的图像文件
	*********/
	strcpy(NameStr,Name);
	strcat(NameStr,".bhd");
	FILE *fp=fopen(NameStr,"rb");
	if(fp==0){ 
		printf("未找到指定文件！\n");
		return 0;
	}

	//跳过位图文件头结构BITMAPFILEHEADER
	fseek(fp, sizeof(BITMAPFILEHEADER),0);

	//定义位图信息头结构变量，读取位图信息头进内存，存放在变量head中
	BITMAPINFOHEADER head;  
	fread(&head, sizeof(BITMAPINFOHEADER), 1,fp); 

	//获取图像宽、高、每像素所占位数等信息
	bmpWidth = head.biWidth;
	bmpHeight = head.biHeight;
	biBitCount = head.biBitCount;

	//定义变量，计算图像每行像素所占的字节数（必须是4的倍数）
	int lineByte=(bmpWidth * biBitCount/8+3)/4*4;

	//灰度图像有颜色表，且颜色表表项为256
	if(biBitCount==8){
		//申请颜色表所需要的空间，读颜色表进内存
		pColorTable=new RGBQUAD[256];
		fread(pColorTable,sizeof(RGBQUAD),256,fp);
	}

	//申请位图数据所需要的空间，读位图数据进内存
	BmpBuf=new unsigned char[lineByte * bmpHeight];
	fread(BmpBuf,1,InfLen / 8,fp);

	//关闭文件
	fclose(fp);

	return 1;
}



void HuffmanDecode()
{
	//获取编码信息
	int i,j,tmp;
	int lineByte=(bmpWidth * biBitCount/8+3)/4*4;
	for(i = 0;i < InfLen / 8;i ++)
	{
		j = i * 8 + 7;
		tmp = *(BmpBuf + i);
		while(tmp > 0)
		{
			ImgInf[j] = tmp % 2;
			tmp /= 2;
			j --;
		}
	}
	

	//解码
	int p = NodeStart;	//遍历指针位置
	j = 0;
	i = 0;
	//do
	while(i <= InfLen)
	{
		if(node[p].gray >= 0)
		{
			*(BmpBuf + j) = node[p].gray;
			//printf("%d ",*(BmpBuf + j));
			j ++;
			p = NodeStart;
		} 
		if(ImgInf[i] == 1)
			p = node[p].lson;	
		else if(ImgInf[i] == 0)
			p = node[p].rson;
		i ++;
	}

}

void main()
{

	int order;//命令 
	int i,j;



	while(1)
	{
		printf("选择以下功能\n\n\t1.BMP图像Huffman编码\n\t2.Huffman编码BMP文件解码\n\t3.退出\n\n请选择需要执行的命令:");
		scanf("%d",&order);

		if(order == 1)
		{ 
			//char readPath[]="F:\学习\图像数据结构与算法\作业\大作业\霍夫曼编码\2.bmp";
			printf("\n---BMP图像Huffman编码---\n");
			printf("\n请输入要编码图像名称：");
			scanf("%s",str);

			//读入指定BMP文件进内存
			char readPath[100];
			strcpy(readPath,str);
			strcat(readPath,".bmp");
			if(readFile(readPath))
			{
				//输出图像的信息
				printf("\n图像信息：\nwidth=%d,height=%d,biBitCount=%d\n",bmpWidth,bmpHeight,biBitCount);

				int lineByte=(bmpWidth * biBitCount/8+3)/4*4;

				if(biBitCount==8)
				{
					//编码初始化
											
					HuffmanCodeInit();

					//计算每个灰度值出现的次数 
					for(i = 0;i < bmpHeight;i ++){
						for(j = 0;j < bmpWidth;j ++)
						{
							lpBuf = (unsigned char *)BmpBuf + lineByte * i + j;
							Num[*(lpBuf)] += 1;
						}
					}

					//调用编码	
					HuffmanCode();

					//将图像数据存盘
					//char writePath[]="F:\学习\图像数据结构与算法\作业\大作业\霍夫曼编码";
					char writePath[100];
					//保存编码后的bmp
					strcpy(writePath,str);
					strcat(writePath,"_.bhd");
					saveBmp(writePath, BmpBuf, bmpWidth, bmpHeight, biBitCount, pColorTable);
					//保存Huffman编码信息和编码树
					strcpy(writePath,str);
					strcat(writePath,"_.bpt");
					saveInfo(writePath,lineByte);
					strcpy(writePath,str);
					strcat(writePath,".txt");
					saveTreeInfo(writePath,lineByte);
					//printf("\n编码信息保存在 %s_Huffman 文件中\n\n",str);
				}
				else
				{
					printf("本程序只支持256色BMP编码！\n");
				}



				//清除缓冲区，BmpBuf和pColorTable是全局变量，在文件读入时申请的空间
				delete []BmpBuf;
				if(biBitCount==8)
					delete []pColorTable;
			}

			printf("\n-----------------------------------------------\n\n\n");
		}
		else if(order == 2)
		{

			printf("\nHuffman编码BMP文件解码\n");
			printf("\n请输入要解码文件名称：");
			scanf("%s",str);
			//编码解码初始化
			HuffmanCodeInit();

			if(readHuffman(str))
			{	//读取文件
				HuffmanDecode();	//Huffman解码

				//将图像数据存盘
				char writePath[100];
				//保存解码后的bmp
				strcpy(writePath,str);
				strcat(writePath,"_Decode.bmp");
				InfLen = bmpWidth * bmpHeight * 8;
				saveBmp(writePath, BmpBuf, bmpWidth, bmpHeight, biBitCount, pColorTable);		
				system(writePath);

				printf("\n保存为 %s_Decode.bmp\n\n",str);
			}
			printf("\n-----------------------------------------------\n\n\n");

		}
		else if(order == 3)
			break;
	}


}