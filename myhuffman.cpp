#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "Windows.h"
#include "math.h"


//����ȫ�ֱ�������Ŷ���ͼ���λͼ���ݡ����ߡ���ɫ��ÿ������ռλ��(����) 
//�˴�����ȫ�ֱ�����ҪΪ�˺����ͼ�����ݷ��ʼ�ͼ��洢��׼��
unsigned char *BmpBuf;//����ͼ�����ݵ�ָ��
int bmpWidth;//ͼ��Ŀ�
int bmpHeight;//ͼ��ĸ�
int imgSpace;//ͼ������ռ�
RGBQUAD *pColorTable;//��ɫ��ָ��
int biBitCount;//ͼ������
char str[100];//�ļ����� 
int Num[300];//���Ҷ�ֵ���ֵĴ��� 
float Feq[300];//���Ҷ�ֵ���ֵ�Ƶ�� 
unsigned char *lpBuf;//ָ��ͼ�����ص�ָ��
unsigned char *m_pDib;//��Ŵ��ļ���DIB


int NodeNum;	//Huffman���ܽڵ����
int NodeStart;	//Huffman����ʼ�ڵ�
struct Node{		//Huffman���ڵ�
	int gray;		//��¼Ҷ�ӽڵ�ĻҶ�ֵ����Ҷ�ӽڵ�Ϊ -1��
	int lson,rson;	//�ڵ�����Ҷ��ӣ���û����Ϊ -1��
	int num;		//�ڵ����ֵ���������ݣ�
	int use;		//��¼�ڵ��Ƿ��ù�(�ù�Ϊ1��û�ù�Ϊ0)
}node[600];

char CodeTmp[300];
char CodeStr[300][300];	//��¼����ֵ
int CodeLen[300];		//���볤��
bool ImgInf[8000000];	//ͼ����Ϣ
int InfLen;				//ͼ����Ϣ����


//��ͼ���ļ��ĺ�����0Ϊʧ��,1Ϊ�ɹ�
bool readFile(char *filePath)
{
	//�����ƶ���ʽ��ָ����ͼ���ļ�
	FILE *fp=fopen(filePath,"rb");
	if(fp==0)
	{ 
		printf("δ�ҵ�ָ���ļ���\n");
		return 0;
	}


	//����λͼ�ļ�ͷ�ṹBITMAPFILEHEADER
	fseek(fp, sizeof(BITMAPFILEHEADER),0);


	//������Ϣͷ
	BITMAPINFOHEADER head;  
	fread(&head, sizeof(BITMAPINFOHEADER), 1,fp); 

	//��ȡͼ����ߡ�ÿ������ռλ������Ϣ
	bmpWidth = head.biWidth;
	bmpHeight = head.biHeight;
	biBitCount = head.biBitCount;

	//�������������ͼ��ÿ��������ռ���ֽ�����������4�ı�����
	int lineByte=(bmpWidth * biBitCount/8+3)/4*4;

	//�Ҷ�ͼ������ɫ������ɫ�����Ϊ256
	if(biBitCount==8)
	{
		//������ɫ������Ҫ�Ŀռ䣬����ɫ����ڴ�
		pColorTable=new RGBQUAD[256];
		fread(pColorTable,sizeof(RGBQUAD),256,fp);
	}

	//����λͼ��������Ҫ�Ŀռ䣬��λͼ���ݽ��ڴ�
	BmpBuf=new unsigned char[lineByte * bmpHeight];
	fread(BmpBuf,1,lineByte * bmpHeight,fp);

	//�ر��ļ�
	fclose(fp);

	return 1;
}
/*********************

Huffman����

*********************/
//Huffman�����ʼ��
void HuffmanCodeInit()
{
	int i;
	for(i = 0;i <256;i ++)//�Ҷ�ֵ��¼����
		Num[i] = 0;
	//��ʼ����������
	for(i = 0;i < 600;i ++)
	{
		node[i].gray = -1;
		node[i].lson  = node[i].rson = -1;
		node[i].num   = -1;
		node[i].use  = 0;
	}
	NodeNum = 0;
}
//�����������Huffman��
void dsearch(int pos,int len)
{
	//���������
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

	//�����Ҷ���
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


//Ѱ��ֵ��С�Ľڵ�
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

//������תʮ����
int Change2to10(int pos){
	int i,j,two = 1;
	j = 0;
	for(i = pos + 7;i >= pos;i --){
		j += two * ImgInf[i];
		two *= 2;
	}
	return j;
}

//Huffman����
void HuffmanCode()
{
	int i,j,k,a,b;

	for(i = 0;i < 256;i ++)
	{//������ʼ�ڵ�
		Feq[i] = (float)Num[i] / (float)(bmpHeight * bmpWidth);//����Ҷ�ֵƵ��
		//Num[i] = (float)Num[i] / (float)(bmpHeight * bmpWidth);//����Ҷ�ֵƵ��
		if(Num[i] > 0)
		{
			node[NodeNum].gray = i;
			node[NodeNum].num = Num[i];
			node[NodeNum].lson = node[NodeNum].rson = -1;	//Ҷ�ӽڵ������Ҷ���
			NodeNum ++;
		}
	}

	while(1)
	{	//�ҵ�����ֵ��С�Ľڵ㣬�ϲ���Ϊ�µĽڵ�
		a = MinNode();
		if(a == -1)
			break;
		b = MinNode();
		if(b == -1)
			break;

		//�����½ڵ�
		node[NodeNum].gray = -1;
		node[NodeNum].num = node[a].num + node[b].num;
		node[NodeNum].lson = a;
		node[NodeNum].rson = b;
		NodeNum ++;
	}

	//���ݽ��õ�Huffman����������������
	dsearch(NodeStart,0);


	//��¼ͼ����Ϣ
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

	//�ٱ�������
	j = 0;
	for(i = 0;i < InfLen;)
	{
		*(BmpBuf + j) = Change2to10(i);
		i += 8;
		j ++;
	}
}

//����ѹ������ļ�
bool saveBmp(char *bmpName, unsigned char *imgBuf, int width, int height, 
			 int biBitCount, RGBQUAD *pColorTable)
{
	//���λͼ����ָ��Ϊ0,��û�����ݴ���,��������
	if(!imgBuf)
		return 0;

	//��ɫ���С,���ֽ�Ϊ��λ,�Ҷ�ͼ����ɫ��Ϊ1024�ֽ�,��ɫͼ����ɫ���СΪ0
	int colorTablesize=0;
	if(biBitCount==8)
		colorTablesize=1024;

	//���洢ͼ������ÿ���ֽ���Ϊ4�ı���
	int lineByte=(width * biBitCount/8+3)/4*4;

	//�Զ�����д�ķ�ʽ���ļ�
	FILE *fp=fopen(bmpName,"wb");
	if(fp==0) return 0;

	//����λͼ�ļ�ͷ�ṹ��������д�ļ�ͷ��Ϣ
	BITMAPFILEHEADER fileHead;
	fileHead.bfType = 0x4D42;//bmp����

	//bfSize��ͼ���ļ�4����ɲ���֮��
	fileHead.bfSize= sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER)
		+ colorTablesize + lineByte*height;
	fileHead.bfReserved1 = 0;
	fileHead.bfReserved2 = 0;

	//bfOffBits��ͼ���ļ�ǰ������������ռ�֮��
	fileHead.bfOffBits=54+colorTablesize;

	//д�ļ�ͷ���ļ�
	fwrite(&fileHead, sizeof(BITMAPFILEHEADER),1, fp);

	//����λͼ��Ϣͷ�ṹ��������д��Ϣͷ��Ϣ
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
	//дλͼ��Ϣͷ���ڴ�
	fwrite(&head, sizeof(BITMAPINFOHEADER),1, fp);

	//����Ҷ�ͼ��,����ɫ��,д���ļ� 
	if(biBitCount==8)
		fwrite(pColorTable, sizeof(RGBQUAD),256, fp);

	//дλͼ���ݽ��ļ�
	//fwrite(imgBuf, height*lineByte, 1, fp);
	fwrite(imgBuf, InfLen / 8, 1, fp);


	//�ر��ļ�
	fclose(fp);

	return 1;
}

//���Huffman������������Ϣ
int saveTreeInfo(char *writePath,int lineByte){
	int i;

	FILE *ftxt;
	ftxt = fopen(writePath,"w");
	//fprintf(fout,"%d %d %d\n",NodeStart,NodeNum,InfLen);//�����ʼ�ڵ㡢�ڵ�������ͼ����ռ�ռ�
	for(i = 0;i < 256;i ++)
	if(Num[i] > 0)
	{		//���Huffman��
	       
           fprintf(ftxt,"�Ҷ�ֵ��%3d  Ƶ��: %f  �볤: %2d  ����: %s\n",i,Feq[i],CodeLen[i],CodeStr[i]);      
	}



	fclose(ftxt);

	return 0;
}

//����Huffman������Ϣ 
int saveInfo(char *writePath,int lineByte){
	int i,j,k;

	FILE *fout;
	fout = fopen(writePath,"w");
	fprintf(fout,"%d %d %d\n",NodeStart,NodeNum,InfLen);//�����ʼ�ڵ㡢�ڵ�������ͼ����ռ�ռ�
	for(i = 0;i <  NodeNum;i ++){		//���Huffman��
            fprintf(fout,"%d %d %d\n",node[i].gray,node[i].lson,node[i].rson);    
	}



	fclose(fout);

	return 0;
}

/*********************

Huffman����ͼ�����

*********************/
//�������ͼ��
bool readHuffman(char *Name)
{
	int i,j;
	char NameStr[100];
	//��ȡHuffman������Ϣ�ͱ�����
	strcpy(NameStr,Name);
	strcat(NameStr,".bpt");
	FILE *fin = fopen(NameStr,"r");
	if(fin == 0){
		printf("δ�ҵ�ָ���ļ���\n");
		return 0;
	}
	fscanf(fin,"%d %d %d",&NodeStart,&NodeNum,&InfLen);
	//printf("%d %d %d\n",NodeStart,NodeNum,InfLen);
	for(i = 0;i < NodeNum;i ++){
		fscanf(fin,"%d %d %d",&node[i].gray,&node[i].lson,&node[i].rson);
		//printf("%d %d %d\n",node[i].color,node[i].lson,node[i].rson);
	}

	/********
	�����ƶ���ʽ��ָ����ͼ���ļ�
	*********/
	strcpy(NameStr,Name);
	strcat(NameStr,".bhd");
	FILE *fp=fopen(NameStr,"rb");
	if(fp==0){ 
		printf("δ�ҵ�ָ���ļ���\n");
		return 0;
	}

	//����λͼ�ļ�ͷ�ṹBITMAPFILEHEADER
	fseek(fp, sizeof(BITMAPFILEHEADER),0);

	//����λͼ��Ϣͷ�ṹ��������ȡλͼ��Ϣͷ���ڴ棬����ڱ���head��
	BITMAPINFOHEADER head;  
	fread(&head, sizeof(BITMAPINFOHEADER), 1,fp); 

	//��ȡͼ����ߡ�ÿ������ռλ������Ϣ
	bmpWidth = head.biWidth;
	bmpHeight = head.biHeight;
	biBitCount = head.biBitCount;

	//�������������ͼ��ÿ��������ռ���ֽ�����������4�ı�����
	int lineByte=(bmpWidth * biBitCount/8+3)/4*4;

	//�Ҷ�ͼ������ɫ������ɫ�����Ϊ256
	if(biBitCount==8){
		//������ɫ������Ҫ�Ŀռ䣬����ɫ����ڴ�
		pColorTable=new RGBQUAD[256];
		fread(pColorTable,sizeof(RGBQUAD),256,fp);
	}

	//����λͼ��������Ҫ�Ŀռ䣬��λͼ���ݽ��ڴ�
	BmpBuf=new unsigned char[lineByte * bmpHeight];
	fread(BmpBuf,1,InfLen / 8,fp);

	//�ر��ļ�
	fclose(fp);

	return 1;
}



void HuffmanDecode()
{
	//��ȡ������Ϣ
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
	

	//����
	int p = NodeStart;	//����ָ��λ��
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

	int order;//���� 
	int i,j;



	while(1)
	{
		printf("ѡ�����¹���\n\n\t1.BMPͼ��Huffman����\n\t2.Huffman����BMP�ļ�����\n\t3.�˳�\n\n��ѡ����Ҫִ�е�����:");
		scanf("%d",&order);

		if(order == 1)
		{ 
			//char readPath[]="F:\ѧϰ\ͼ�����ݽṹ���㷨\��ҵ\����ҵ\����������\2.bmp";
			printf("\n---BMPͼ��Huffman����---\n");
			printf("\n������Ҫ����ͼ�����ƣ�");
			scanf("%s",str);

			//����ָ��BMP�ļ����ڴ�
			char readPath[100];
			strcpy(readPath,str);
			strcat(readPath,".bmp");
			if(readFile(readPath))
			{
				//���ͼ�����Ϣ
				printf("\nͼ����Ϣ��\nwidth=%d,height=%d,biBitCount=%d\n",bmpWidth,bmpHeight,biBitCount);

				int lineByte=(bmpWidth * biBitCount/8+3)/4*4;

				if(biBitCount==8)
				{
					//�����ʼ��
											
					HuffmanCodeInit();

					//����ÿ���Ҷ�ֵ���ֵĴ��� 
					for(i = 0;i < bmpHeight;i ++){
						for(j = 0;j < bmpWidth;j ++)
						{
							lpBuf = (unsigned char *)BmpBuf + lineByte * i + j;
							Num[*(lpBuf)] += 1;
						}
					}

					//���ñ���	
					HuffmanCode();

					//��ͼ�����ݴ���
					//char writePath[]="F:\ѧϰ\ͼ�����ݽṹ���㷨\��ҵ\����ҵ\����������";
					char writePath[100];
					//���������bmp
					strcpy(writePath,str);
					strcat(writePath,"_.bhd");
					saveBmp(writePath, BmpBuf, bmpWidth, bmpHeight, biBitCount, pColorTable);
					//����Huffman������Ϣ�ͱ�����
					strcpy(writePath,str);
					strcat(writePath,"_.bpt");
					saveInfo(writePath,lineByte);
					strcpy(writePath,str);
					strcat(writePath,".txt");
					saveTreeInfo(writePath,lineByte);
					//printf("\n������Ϣ������ %s_Huffman �ļ���\n\n",str);
				}
				else
				{
					printf("������ֻ֧��256ɫBMP���룡\n");
				}



				//�����������BmpBuf��pColorTable��ȫ�ֱ��������ļ�����ʱ����Ŀռ�
				delete []BmpBuf;
				if(biBitCount==8)
					delete []pColorTable;
			}

			printf("\n-----------------------------------------------\n\n\n");
		}
		else if(order == 2)
		{

			printf("\nHuffman����BMP�ļ�����\n");
			printf("\n������Ҫ�����ļ����ƣ�");
			scanf("%s",str);
			//��������ʼ��
			HuffmanCodeInit();

			if(readHuffman(str))
			{	//��ȡ�ļ�
				HuffmanDecode();	//Huffman����

				//��ͼ�����ݴ���
				char writePath[100];
				//���������bmp
				strcpy(writePath,str);
				strcat(writePath,"_Decode.bmp");
				InfLen = bmpWidth * bmpHeight * 8;
				saveBmp(writePath, BmpBuf, bmpWidth, bmpHeight, biBitCount, pColorTable);		
				system(writePath);

				printf("\n����Ϊ %s_Decode.bmp\n\n",str);
			}
			printf("\n-----------------------------------------------\n\n\n");

		}
		else if(order == 3)
			break;
	}


}