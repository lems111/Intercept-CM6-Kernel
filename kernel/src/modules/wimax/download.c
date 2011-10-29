#include "headers.h"
#include "download.h"
#include "firmware.h"
#include <linux/vmalloc.h>

IMAGE_DATA g_stWiMAXImage;
extern DWORD g_dwLineState;

int	WimaxMode(void)
{
	struct file *fp;		
	int mode = -1;

	fp = klib_fopen("/proc/wmxmode/mode", O_RDONLY, 0);
	if(fp != NULL) 
	{
		mode = klib_fgetc(fp);
		DumpDebug(HARDWARE, "Mode = %d", mode);
		klib_fclose(fp);
	}
	else
	{
		DumpDebug(HARDWARE, "Mode access failed!!!");
	}
	return mode;
}

int LoadWiMaxImage(void)
{	
	DWORD dwImgSize;		
	unsigned long buforder;
	struct file *fp;	
	int read_size = 0;

	if ( g_dwLineState == AUTH_MODE)
	{
		fp = klib_fopen(WIMAX_LOADER_PATH, O_RDONLY, 0);
	}
	else
	{
		fp = klib_fopen(WIMAX_IMAGE_PATH, O_RDONLY, 0);
	}

	if(fp)
	{
//		dwImgSize = klib_flength(fp);
//		buforder = get_order(dwImgSize);		
		//DumpDebug(HARDWARE, "LoadWiMAXImage ..");

//		g_stWiMAXImage.pImage = (char *) vmalloc(dwImgSize);//__get_free_pages(GFP_KERNEL, buforder);
		g_stWiMAXImage.pImage = (char *) vmalloc(MAX_WIMAXFW_SIZE);//__get_free_pages(GFP_KERNEL, buforder);
		if(!g_stWiMAXImage.pImage)
		{
			DumpDebug(HARDWARE, "Error: Memory alloc failure");
			klib_fclose(fp);
			return STATUS_UNSUCCESSFUL;
		}

		memset(g_stWiMAXImage.pImage, 0, MAX_WIMAXFW_SIZE);				
		read_size = klib_flen_fcopy(g_stWiMAXImage.pImage, MAX_WIMAXFW_SIZE, fp);
//		read_size = klib_fread(g_stWiMAXImage.pImage, dwImgSize, fp);
		g_stWiMAXImage.uiSize = read_size;
		g_stWiMAXImage.uiWorkAddress = CMC730_WIBRO_ADDRESS;
		g_stWiMAXImage.uiOffset = 0;
		g_stWiMAXImage.buforder = buforder;

		klib_fclose(fp);
	}
	else {
		DumpDebug(HARDWARE, "Error: WiMAX image file open failed");
		return STATUS_UNSUCCESSFUL;
	}
	//DumpDebug(1, "LoadWiMAXImage ---");
	return STATUS_SUCCESS;
}

void UnLoadWiMaxImage(void)
{
	ENTER;
	if(g_stWiMAXImage.pImage != NULL)
	{
		DumpDebug(HARDWARE, "Delete the Image Loaded");
		//free_pages((unsigned long)g_stWiMAXImage.pImage, g_stWiMAXImage.buforder);
		vfree(g_stWiMAXImage.pImage);
		g_stWiMAXImage.pImage = NULL;
	}
	LEAVE;
}

BOOLEAN SendCmdPacket(PMINIPORT_ADAPTER Adapter, unsigned short uiCmdId)
{
	PHW_PACKET_HEADER pPktHdr;
	UCHAR pTxPkt[CMD_MSG_TOTAL_LENGTH];
	UINT uiBufOffset;
	PWIMAX_MESSAGE_HEADER pWibroMsgHdr;
	int status = 0;
	UINT dwTxSize;

	pPktHdr = (PHW_PACKET_HEADER)pTxPkt;
	pPktHdr->Id0 = 'W';
	pPktHdr->Id1 = 'C';	
	pPktHdr->Length = be16_to_cpu(CMD_MSG_TOTAL_LENGTH); 

	uiBufOffset = sizeof(HW_PACKET_HEADER);
	pWibroMsgHdr = (PWIMAX_MESSAGE_HEADER)(pTxPkt + uiBufOffset);
	pWibroMsgHdr->MsgType = be16_to_cpu(ETHERTYPE_DL);
	pWibroMsgHdr->MsgID = be16_to_cpu(uiCmdId);	
	pWibroMsgHdr->MsgLength = be32_to_cpu(CMD_MSG_LENGTH);
		
	dwTxSize = CMD_MSG_TOTAL_LENGTH;

	status = sd_send(Adapter, pTxPkt, dwTxSize);
	if(status != STATUS_SUCCESS) {
		// crc error or data error - set PCWRT '1' & send current type A packet again		
		DumpDebug(FW_DNLD, "hwSdioWrite : crc error");
		return status;//goto rewrite;
	}
	return status;
}

BOOLEAN SendImageInfoPacket(PMINIPORT_ADAPTER Adapter, unsigned short uiCmdId)
{
	PHW_PACKET_HEADER pPktHdr;
	UCHAR pTxPkt[IMAGE_INFO_MSG_TOTAL_LENGTH];
	UINT pImageInfo[4];
	UINT uiBufOffset;
	PWIMAX_MESSAGE_HEADER pWibroMsgHdr;
	int status;
	UINT dwTxSize;

	pPktHdr = (PHW_PACKET_HEADER)pTxPkt;
	pPktHdr->Id0 = 'W';
	pPktHdr->Id1 = 'C';	
	pPktHdr->Length = be16_to_cpu(IMAGE_INFO_MSG_TOTAL_LENGTH); 

	uiBufOffset = sizeof(HW_PACKET_HEADER);
	pWibroMsgHdr = (PWIMAX_MESSAGE_HEADER)(pTxPkt + uiBufOffset);
	pWibroMsgHdr->MsgType = be16_to_cpu(ETHERTYPE_DL);
	pWibroMsgHdr->MsgID = be16_to_cpu(uiCmdId);	
	pWibroMsgHdr->MsgLength = be32_to_cpu(IMAGE_INFO_MSG_LENGTH);

	pImageInfo[0] = 0;
	pImageInfo[1] = be32_to_cpu(g_stWiMAXImage.uiSize);
	pImageInfo[2] = be32_to_cpu(g_stWiMAXImage.uiWorkAddress);
	pImageInfo[3] = 0;

	uiBufOffset += sizeof(WIMAX_MESSAGE_HEADER);
	memcpy(&(pTxPkt[uiBufOffset]), pImageInfo, sizeof(pImageInfo));

	dwTxSize = IMAGE_INFO_MSG_TOTAL_LENGTH;

	status = sd_send(Adapter, pTxPkt, dwTxSize);

	if(status != STATUS_SUCCESS) {
		// crc error or data error - set PCWRT '1' & send current type A packet again		
		DumpDebug(FW_DNLD, "hwSdioWrite : crc error");
		return status;//goto rewrite;
	}
	return status;
}


BOOLEAN SendImageDataPacket(PMINIPORT_ADAPTER Adapter, unsigned short uiCmdId)
{
	PHW_PACKET_HEADER pPktHdr;
	UCHAR pTxPkt[MAX_IMAGE_DATA_MSG_LENGTH];
	UINT uiPayloadSize;
	UINT uiBufOffset;
	UINT uiImageLength;
	PIMAGE_DATA_PAYLOAD pImageDataPayload;
	PWIMAX_MESSAGE_HEADER pWibroMsgHdr;
	int status;
	UINT dwTxSize;

	pPktHdr = (PHW_PACKET_HEADER)pTxPkt;
	pPktHdr->Id0 = 'W';
	pPktHdr->Id1 = 'C';	

	uiBufOffset = sizeof(HW_PACKET_HEADER);
	pWibroMsgHdr = (PWIMAX_MESSAGE_HEADER)(pTxPkt + uiBufOffset);
	pWibroMsgHdr->MsgType = be16_to_cpu(ETHERTYPE_DL);
	pWibroMsgHdr->MsgID = be16_to_cpu(uiCmdId);	

	if(g_stWiMAXImage.uiOffset < (g_stWiMAXImage.uiSize - MAX_IMAGE_DATA_LENGTH))
		uiImageLength = MAX_IMAGE_DATA_LENGTH;
	else
		uiImageLength = g_stWiMAXImage.uiSize - g_stWiMAXImage.uiOffset;		
		
	uiBufOffset += sizeof(WIMAX_MESSAGE_HEADER);
	pImageDataPayload = (PIMAGE_DATA_PAYLOAD)(pTxPkt + uiBufOffset);
	pImageDataPayload->uiOffset = be32_to_cpu( g_stWiMAXImage.uiOffset);
	pImageDataPayload->uiLength = be32_to_cpu(uiImageLength);

	memcpy(pImageDataPayload->ucImageData, 
		      g_stWiMAXImage.pImage + g_stWiMAXImage.uiOffset, 
		      uiImageLength);

	uiPayloadSize = uiImageLength + 8; // PayloadÀÇ Offset + Length + ImageData ±æÀÌ
	pPktHdr->Length = be16_to_cpu(CMD_MSG_TOTAL_LENGTH + uiPayloadSize); 
	pWibroMsgHdr->MsgLength = be32_to_cpu(uiPayloadSize);

	dwTxSize = CMD_MSG_TOTAL_LENGTH + uiPayloadSize;

#if 0
	{
				// dump packets
				UINT i, l = dwTxSize;				
				RETAILMSG(1, (TEXT("Send Image Data packet [%d] = "), l));
				for (i = 0; i < l; i++) {
					RETAILMSG(1, (TEXT("%02x"), pTxPkt[i]));
					if (i != (l - 1)) 
						RETAILMSG(1, (_T(",")));
					if ((i != 0) && ((i%32) == 0)) RETAILMSG(1, (_T("\n")));
				}
				RETAILMSG(1, (_T("\n")));
	}			
#endif

	status = sd_send(Adapter, pTxPkt, dwTxSize);

	if(status != STATUS_SUCCESS) {
		// crc error or data error - set PCWRT '1' & send current type A packet again		
		DumpDebug(FW_DNLD, "hwSdioWrite : crc error");
		return status;//goto rewrite;
	}
	g_stWiMAXImage.uiOffset += uiImageLength;

	return status;		
}
