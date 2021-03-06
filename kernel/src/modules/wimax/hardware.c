#include "headers.h"
#include "download.h"
#include "ctl_funcs.h"

#include <mach/gpio.h>
#include <plat/gpio-cfg.h>
#include <mach/hardware.h>

#include "wimax_plat.h" //cky 20100225

DWORD	g_dwLineState = SDIO_MODE;
DWORD DatadwValue = 0;
DWORD ControldwValue = 0;

VOID GetMacAddress(MINIPORT_ADAPTER *Adapter);
VOID GetDeviceFeatures(MINIPORT_ADAPTER    *Adapter);
extern BOOLEAN SendCmdPacket(PMINIPORT_ADAPTER Adapter, unsigned short uiCmdId);
extern int WimaxMode(void);
void hwSetInterface(VOID);
BOOLEAN hwGPIOInit(VOID);
BOOLEAN hwGPIODeInit(VOID);

VOID hwGetMacAddressThread(VOID *data)
{
	PMINIPORT_ADAPTER Adapter = (MINIPORT_ADAPTER *)data;
	ENTER;
	mdelay(300);
	GetMacAddress(Adapter);
	LEAVE;
}

void hwSetInterface()
{
	ENTER;

	g_dwLineState = WimaxMode();

	if(g_dwLineState == SDIO_MODE)
	{	
		gpio_set_value(WIMAX_WAKEUP, GPIO_LEVEL_HIGH);	
		gpio_set_value(WIMAX_IF_MODE0, GPIO_LEVEL_HIGH);	

		DatadwValue = 1;
		ControldwValue = 1;
		DumpDebug(HARDWARE, "Interface mode SDIO_MODE");
	}
	else if(g_dwLineState == WTM_MODE || g_dwLineState == AUTH_MODE)
	{
		gpio_set_value(WIMAX_WAKEUP, GPIO_LEVEL_LOW);	
		gpio_set_value(WIMAX_IF_MODE0, GPIO_LEVEL_LOW);

		mdelay(10);
		DatadwValue = 0;
		ControldwValue = 0;
		DumpDebug(HARDWARE, "Interface mode WTM_MODE || AUTH_MODE");
	}
	else if(g_dwLineState == DM_MODE)
	{
		gpio_set_value(WIMAX_WAKEUP, GPIO_LEVEL_HIGH);	
		gpio_set_value(WIMAX_IF_MODE0, GPIO_LEVEL_LOW);	

		DumpDebug(HARDWARE, "Interface mode DM_MODE");
	}
	else if(g_dwLineState == USB_MODE || g_dwLineState ==  USIM_RELAY_MODE)
	{
		gpio_set_value(WIMAX_WAKEUP, GPIO_LEVEL_LOW);	
		gpio_set_value(WIMAX_IF_MODE0, GPIO_LEVEL_HIGH);	

		mdelay(10);
		DatadwValue = 1;
		ControldwValue = 0;
		DumpDebug(HARDWARE, "Interface mode USB_MODE (USIM_RELAY)");
	}

	LEAVE;
}

BOOLEAN hwGPIOInit(void)
{
	ENTER;
	
	// Init each pin -> set function and pull disable
	s3c_gpio_cfgpin(WIMAX_WAKEUP, S3C_GPIO_SFN(1));	// set MEM0 interface
	s3c_gpio_setpull(WIMAX_WAKEUP, S3C_GPIO_PULL_NONE);

	s3c_gpio_cfgpin(WIMAX_IF_MODE0, S3C_GPIO_SFN(1));
	s3c_gpio_setpull(WIMAX_IF_MODE0, S3C_GPIO_PULL_NONE);

	s3c_gpio_cfgpin(WIMAX_IF_MODE1, S3C_GPIO_SFN(1));
	s3c_gpio_setpull(WIMAX_IF_MODE1, S3C_GPIO_PULL_NONE);
	
	s3c_gpio_cfgpin(GPIO_WIMAX_CON0, S3C_GPIO_INPUT);
	s3c_gpio_setpull(GPIO_WIMAX_CON0, S3C_GPIO_PULL_NONE);

	s3c_gpio_cfgpin(GPIO_WIMAX_CON1, S3C_GPIO_SFN(1));
	s3c_gpio_setpull(GPIO_WIMAX_CON1, S3C_GPIO_PULL_NONE);

	// WIMAX_INT set Input and Pull up
	s3c_gpio_cfgpin(GPIO_WIMAX_INT, S3C_GPIO_INPUT);
	s3c_gpio_setpull(GPIO_WIMAX_INT, S3C_GPIO_PULL_UP);

	// Wakeup pin high
	gpio_set_value(WIMAX_WAKEUP, GPIO_LEVEL_HIGH);	
	
	// Set Interface
	hwSetInterface();

	s3c_gpio_cfgpin(GPIO_WIMAX_CON1, S3C_GPIO_SFN(1));
	gpio_set_value(GPIO_WIMAX_CON1, GPIO_LEVEL_HIGH);	


	LEAVE;
	return TRUE;
}

BOOLEAN hwGPIODeInit()
{
	ENTER;
	
	// Init each pin -> set function output, low, and pull disable
	s3c_gpio_cfgpin(WIMAX_WAKEUP, S3C_GPIO_SFN(1));
	gpio_set_value(WIMAX_WAKEUP, GPIO_LEVEL_LOW);	
	s3c_gpio_setpull(WIMAX_WAKEUP, S3C_GPIO_PULL_NONE);

	s3c_gpio_cfgpin(WIMAX_IF_MODE0, S3C_GPIO_SFN(1));
	gpio_set_value(WIMAX_IF_MODE0, GPIO_LEVEL_LOW);	
	s3c_gpio_setpull(WIMAX_IF_MODE0, S3C_GPIO_PULL_NONE);

	s3c_gpio_cfgpin(WIMAX_IF_MODE1, S3C_GPIO_SFN(1));
	gpio_set_value(WIMAX_IF_MODE1, GPIO_LEVEL_LOW);	
	s3c_gpio_setpull(WIMAX_IF_MODE1, S3C_GPIO_PULL_NONE);
	
	s3c_gpio_cfgpin(GPIO_WIMAX_CON0, S3C_GPIO_SFN(1));
	gpio_set_value(GPIO_WIMAX_CON0, GPIO_LEVEL_LOW);	
	s3c_gpio_setpull(GPIO_WIMAX_CON0, S3C_GPIO_PULL_NONE);

	s3c_gpio_cfgpin(GPIO_WIMAX_CON1, S3C_GPIO_SFN(1));
	gpio_set_value(GPIO_WIMAX_CON1, GPIO_LEVEL_LOW);	
	s3c_gpio_setpull(GPIO_WIMAX_CON1, S3C_GPIO_PULL_NONE);

	s3c_gpio_setpull(GPIO_WIMAX_INT, S3C_GPIO_PULL_NONE);
	gpio_set_value(GPIO_WIMAX_INT, GPIO_LEVEL_LOW);	
	s3c_gpio_cfgpin(GPIO_WIMAX_INT, S3C_GPIO_SFN(1));
		
	LEAVE;
	return TRUE;
}

//unsigned long wimax_download_start_time;

INT hwStart(MINIPORT_ADAPTER *Adapter)
{
	ENTER;

	if(LoadWiMaxImage())
			return STATUS_UNSUCCESSFUL;
	
	Adapter->WibroStatus = WIBRO_STATE_READY;
	Adapter->IPRefreshing = FALSE;
	Adapter->bFWDNEndFlag = FALSE;
	
	if(Adapter->DownloadMode)
	{
//		wimax_download_start_time = jiffies;
		sdio_claim_host(Adapter->func);
		SendCmdPacket(Adapter, MSG_DRIVER_OK_REQ);
		sdio_release_host(Adapter->func);
		switch( wait_event_interruptible_timeout(Adapter->hFWDNEndEvent, (Adapter->bFWDNEndFlag == TRUE), msecs_to_jiffies(FWDOWNLOAD_TIMEOUT) ) ) 
		{
			// timeout
			case 0:
				Adapter->bHaltPending = TRUE;
				DumpDebug(HARDWARE, "Error hwStart :  F/W Download timeout failed");
				return STATUS_UNSUCCESSFUL;
			
			// Interrupted by signal
			case -ERESTARTSYS:
				DumpDebug(HARDWARE, "Error hwStart :  -ERESTARTSYS retry");
				return STATUS_UNSUCCESSFUL;

			// normal condition check	
			default:
				if (Adapter->SurpriseRemoval == TRUE)
				{							
					DumpDebug(HARDWARE, "Error hwStart :  F/W Download surprise removed");
					return STATUS_UNSUCCESSFUL;
				}
				DumpDebug(HARDWARE, "hwStart :  F/W Download Complete");
				break;
		}
		Adapter->DownloadMode = FALSE;
	}
	LEAVE;
	return STATUS_SUCCESS;
}

// stop hw 
INT hwStop(MINIPORT_ADAPTER *Adapter)
{
	ENTER;
	
	Adapter->bHaltPending = TRUE;
	// Stop Sdio Interface
	sdio_claim_host(Adapter->func);
	sdio_release_irq(Adapter->func);
	sdio_disable_func(Adapter->func);
	sdio_release_host(Adapter->func);

	LEAVE;
	return STATUS_SUCCESS;
}

INT hwInit(MINIPORT_ADAPTER *Adapter)
{
	ENTER;

	if(!hwGPIOInit())
	{
		DumpDebug(DISPATCH, "hwInit: Can't intialize GPIO");
		return STATUS_UNSUCCESSFUL;
	}

	memset(&Adapter->hw,0,sizeof(HARDWARE_INFO));
	Adapter->hw.ReceiveBuffer= kmalloc(SDIO_BUFFER_SIZE, GFP_ATOMIC);
	if(Adapter->hw.ReceiveBuffer ==NULL) {
		return -ENOMEM;
	}
	Adapter->hw.ReceiveTempBuffer= kmalloc(SDIO_BUFFER_SIZE, GFP_ATOMIC);
	if(Adapter->hw.ReceiveTempBuffer ==NULL) {
			if(Adapter->hw.ReceiveBuffer)
				kfree(Adapter->hw.ReceiveTempBuffer);
		return -ENOMEM;
	}
	// For sending data and control packets
	QueueInitList(Adapter->hw.Q_Send.Head);
	spin_lock_init(&Adapter->hw.Q_Send.Lock);
	
	INIT_WORK(&Adapter->work, hwTransmitThread);
	init_waitqueue_head(&Adapter->hFWDNEndEvent);

	init_completion(&Adapter->hAwakeAckEvent);
	
	return STATUS_SUCCESS;
	LEAVE;
}

VOID hwRemove(MINIPORT_ADAPTER *Adapter)
{
	PBUFFER_DESCRIPTOR  dsc;
    ENTER;

	//sangam :Free the pending data packets and control packets
	while(!QueueEmpty(Adapter->hw.Q_Send.Head)) {	//sangam dbg : used only for data packet so free skb
		DumpDebug(DISPATCH, "<1> Freeing Q_Send");
		dsc = (PBUFFER_DESCRIPTOR) QueueGetHead(Adapter->hw.Q_Send.Head);
		if (!dsc) {
			DumpDebug(DISPATCH, "<1> Fail...node is null");
			continue;
		}
		QueueRemoveHead(Adapter->hw.Q_Send.Head);
		if(dsc->Buffer)
			kfree(dsc->Buffer);
		if(dsc)
			kfree(dsc);
	}
	// stop data out buffer
	if (Adapter->hw.ReceiveBuffer!= NULL) 
			kfree(Adapter->hw.ReceiveBuffer);
	// stop TempData out buffer
	if (Adapter->hw.ReceiveTempBuffer!= NULL) 
			kfree(Adapter->hw.ReceiveTempBuffer);

	hwGPIODeInit();	
	LEAVE;
}

#ifdef DISCONNECT_TIMER
void  hwInitDisconnectTimer(MINIPORT_ADAPTER *Adapter)
{
	ENTER;
	Adapter->DisconnectTimerActivated = FALSE;
   	Adapter->DisconnectTimer.function = hwDisconnectTimerHandler; 
	Adapter->DisconnectTimer.data = (unsigned long) Adapter; 
	Adapter->DisconnectTimer.expires = jiffies + (HZ * 100 / HARDWARE_DISCONNECT_TIMEOUT); // 300 msec 
	LEAVE;
}
void  hwStartDisconnectTimer(MINIPORT_ADAPTER *Adapter)
{
	ENTER;
	if(!Adapter->DisconnectTimerActivated)
	{
		add_timer(&Adapter->DisconnectTimer);	
		Adapter->DisconnectTimerActivated = TRUE;
	}
	LEAVE;
}
void hwStopDisconnectTimer(MINIPORT_ADAPTER *Adapter)
{
	ENTER;
	if(Adapter->DisconnectTimerActivated)
	{
		del_timer(&Adapter->DisconnectTimer);	
		Adapter->DisconnectTimerActivated = FALSE;
	}
	LEAVE;
}
void hwDisconnectTimerHandler(unsigned long ptr)
{
	MINIPORT_ADAPTER *Adapter=(struct MINIPORT_ADAPTER *)ptr;
	ENTER;
	if(!Adapter->bHaltPending)
	{
		if (Adapter->MediaState != MEDIA_DISCONNECTED) {
			Adapter->MediaState = MEDIA_DISCONNECTED;
			netif_stop_queue(Adapter->net);
		}
	}
	Adapter->DisconnectTimerActivated = FALSE;
	LEAVE;			
}	
#endif

/* get MAC address from device */
VOID GetMacAddress(MINIPORT_ADAPTER *Adapter)
{
	UINT nCount = 0;
	int nResult = 0;
	HW_PRIVATE_PACKET req; 
	ENTER;

	req.Id0   = 'W';
	req.Id1   = 'P';
	req.Code  = HwCodeMacRequest;
	req.Value = 0;
	Adapter->acquired_mac_address = FALSE;
	
	do {
		if(Adapter->bHaltPending)
			return;
		sdio_claim_host(Adapter->func);
		nResult = sdio_memcpy_toio(Adapter->func,SDIO_DATA_PORT_REG, &req, sizeof(HW_PRIVATE_PACKET));	
		sdio_release_host(Adapter->func);
		if(nResult != 0) 
			DumpDebug(DRV_ENTRY,"Send GetMacAddress Request msg but error occurred!! res = %d",nResult);

//		sdio_release_host(Adapter->func);
		if(!wait_for_completion_interruptible_timeout(&Adapter->hFWInitCompleteEvent, msecs_to_jiffies(HARDWARE_START_TIMEOUT))) {
			DumpDebug(DRV_ENTRY, "timedout mac req, retry..");
			if(nCount >= HARDWARE_MAX_MAC_RESPONSES) {
				DumpDebug(DRV_ENTRY,"Can't get mac address, exceeded number of retries (%d)",nCount);
				Adapter->bHaltPending = TRUE;
				break;
			}
			nCount++;
			continue;
		}
		// If wait exit before timeout then mac assigned or surprise remove
		if(Adapter->acquired_mac_address == TRUE) {
			DumpDebug(DRV_ENTRY,"MAC Address acquired");
			break;
		}
//		mdelay(500);
	
	} while(TRUE);

	return;
	LEAVE;
}

unsigned int hwSendData(MINIPORT_ADAPTER *Adapter, void *Buffer , u_long Length)
{
	PUCHAR pTempBuf;
	PHW_PACKET_HEADER hdr;
	PBUFFER_DESCRIPTOR dsc;
	struct net_device *net = Adapter->net;
	ULONG	lCount = 0;

	ENTER;

	lCount = bufFindCount(Adapter->hw.Q_Send.Head);
	if(lCount > Adapter->FullQLimit) {
		DumpDebug(MP_SEND, "Queue count (%lu) exceeds queue limit (%lu)", lCount, Adapter->FullQLimit);
		return 0;
	}

	// send response to stack if sent packet is ARP packet
	if (ethIsArpRequest(Buffer, Length)) {
		ethSendArpResponse(Adapter, Buffer, Length);
		DumpDebug(MP_SEND, "Sent ARP Response");	// sangam : Need freeing the allocated memory or OS will do????
		return STATUS_SUCCESS;
	}

	dsc = (PBUFFER_DESCRIPTOR) kmalloc(sizeof(BUFFER_DESCRIPTOR), GFP_ATOMIC); 
	if(dsc == NULL){ 
		return STATUS_RESOURCES;
	}

	dsc->Buffer = kmalloc(BUFFER_DATA_SIZE ,GFP_ATOMIC);  
	if(dsc->Buffer == NULL) {
		kfree(dsc);
		return STATUS_RESOURCES;
	}
	 
	pTempBuf= dsc->Buffer;
// shift data pointer
	pTempBuf += sizeof(HW_PACKET_HEADER);
#ifdef HARDWARE_USE_ALIGN_HEADER
		pTempBuf += 2;
#endif
	hdr = (PHW_PACKET_HEADER)dsc->Buffer;

	Length -= (MINIPORT_LENGTH_OF_ADDRESS * 2);
	Buffer += (MINIPORT_LENGTH_OF_ADDRESS * 2);

	memcpy(pTempBuf, Buffer, Length);

	hdr->Id0	 = 'W';
	hdr->Id1	 = 'D';
	hdr->Length = (USHORT)Length; 

	dsc->data.Length = Length + sizeof(HW_PACKET_HEADER);
#ifdef HARDWARE_USE_ALIGN_HEADER
	dsc->data.Length += 2;
#endif		
	// add statistics
	Adapter->stats.tx.Count += 1;
	Adapter->stats.tx.Bytes += dsc->data.Length;

	Adapter->netstats.tx_packets++;
	Adapter->netstats.tx_bytes += dsc->data.Length;
	Adapter->stats.tx.Count++;
	
	QueuePutTail(Adapter->hw.Q_Send.Head, dsc->Node);
	schedule_work(&Adapter->work);
	// set flag - wait RxReady
	Adapter->hw.SendDisabled = TRUE;
	
	if(!netif_running(net)){
		DumpDebug(DRV_ENTRY, "!netif_running");
	}
	
	LEAVE;
	return STATUS_SUCCESS;
}

unsigned int SendDataOut(MINIPORT_ADAPTER *Adapter, PBUFFER_DESCRIPTOR dsc)
{
	int nRet = 0;
	ENTER;
	// sangam dbg : For control packet padding
	dsc->data.Length  += (dsc->data.Length & 1) ? 1: 0;
	
	//DumpDebug(MP_SEND, "sangam dbg  before padding len = %ld", dsc->data.Length);
#ifdef HARDWARE_USE_ALIGN_HEADER
	if(dsc->data.Length > SDIO_MAX_BYTE_SIZE)
		dsc->data.Length = (dsc->data.Length +(SDIO_MAX_BYTE_SIZE -1)) & ~(SDIO_MAX_BYTE_SIZE -1);
#endif

	if (Adapter->bHaltPending) {
		DumpDebug(DRV_ENTRY, "Halted Already");
		return STATUS_UNSUCCESSFUL;
	}
	nRet = sdio_memcpy_toio(Adapter->func, SDIO_DATA_PORT_REG, dsc->Buffer, dsc->data.Length);	
	if(nRet < 0) {
		DumpDebug(DRV_ENTRY,"SendOut but error occurred!! nRet = %d",nRet);
	}
	//else	
	//	DumpDebug(DRV_ENTRY, "Tx length = %ld",dsc->data.Length);
#if 0
	{
	    // dump packets
		UINT i;
		PUCHAR  b = (PUCHAR)dsc->Buffer;
		DumpDebug(MP_SEND, "Sent packet LEN = %ld\n", dsc->data.Length);
	    DumpDebug(MP_SEND, "Sent packet = ");
	    for (i = 0; i < dsc->data.Length; i++) {
		DumpDebug(MP_SEND, "%02x", b[i]);
		if (i != (dsc->data.Length - 1)) DumpDebug(MP_SEND, ",");
		if ((i != 0) && ((i%32) == 0)) DumpDebug(MP_SEND, "\n");
	    }
	    DumpDebug(MP_SEND, "\n");
	}
#endif
	LEAVE;
	return nRet;
}

// used only during firmware download
unsigned int sd_send(MINIPORT_ADAPTER *Adapter, UCHAR* pBuffer, UINT cbBuffer)
{
	int nRet = 0;
	PUCHAR buf = (PUCHAR) pBuffer;
	UINT size=cbBuffer;
//	UINT remainder=0;

	cbBuffer  += (cbBuffer & 1) ? 1: 0;

	if (Adapter->bHaltPending || Adapter->SurpriseRemoval) {
		DumpDebug(FW_DNLD, "Halted Already");
		return STATUS_UNSUCCESSFUL;
	}
	
	nRet = sdio_memcpy_toio(Adapter->func, SDIO_DATA_PORT_REG, pBuffer, cbBuffer);	

	if(nRet < 0) {
		DumpDebug(FW_DNLD,"SendOut but error occurred!! nRet = %d",nRet);
	}
	//else
	//	DumpDebug(FW_DNLD, "Tx length = %d",cbBuffer);

#if 0
		{
	    	// dump packets
			UINT i;
			PUCHAR  b = (PUCHAR)pBuffer;
			DumpDebug(MP_SEND, "Sent packet LEN = %ld\n",cbBuffer);
	    		DumpDebug(MP_SEND, "Sent packet = ");
	    		for (i = 0; i < cbBuffer; i++) {
				DumpDebug(MP_SEND, "%02x", b[i]);
				if (i != (cbBuffer - 1)) printk(",");
				if ((i != 0) && ((i%32) == 0)) printk("\n");
	    		}
	    		DumpDebug(MP_SEND, "\n");
		}
#endif

	return nRet;
}

#ifdef  WAKEUP_BY_GPIO
INT hwDeviceWakeup(PMINIPORT_ADAPTER pAdapter)
{
	UCHAR buf[512];
	ULONG nlength = 0;
	UCHAR retryCount=0;
	int rc = 0;

	if(pAdapter->WibroStatus == WIBRO_STATE_READY) {
		DumpDebug(HARDWARE, "wibro status = %d", pAdapter->WibroStatus);
		return 0;
	}

	pAdapter->Previous_WibroStatus= pAdapter->WibroStatus;        
	pAdapter->WibroStatus = WIBRO_STATE_AWAKE_REQUESTED;

	DumpDebug(HARDWARE, "WIBRO_STATE_AWAKE_REQUESTED hwDeviceWakeup_State : %d", pAdapter->Previous_WibroStatus);
	
	// try to wake up modem MAX_TRY times
	while(retryCount < WAKEUP_MAX_TRY)
	{
		// set wake up gpio
		s3c_gpio_cfgpin(WIMAX_WAKEUP, S3C_GPIO_SFN(1));
		gpio_set_value(WIMAX_WAKEUP, GPIO_LEVEL_LOW);	//Nwake up pin Low
		rc = wait_for_completion_interruptible_timeout(&pAdapter->hAwakeAckEvent, msecs_to_jiffies(WAKEUP_TIMEOUT) );

		gpio_set_value(WIMAX_WAKEUP, GPIO_LEVEL_HIGH);	 //Nwake up pin high
		if(rc){
			DumpDebug(HARDWARE, " hwDeviceWakeup --Received Wake Up Ack !!");
			break;
		}
		else
		{
			DumpDebug(HARDWARE,"hwDeviceWakeup --Wake Up Ack Timeout! retryCount = %d--", retryCount);
		}
		retryCount++;
	}

	//if (retryCount == WAKEUP_MAX_TRY)
	{
		DumpDebug(HARDWARE,	"hwDeviceWakeup retryCount = %d -GPIO Regs=%d !!", retryCount, gpio_get_value(GPIO_WIMAX_CON0) );
		DumpDebug(HARDWARE, "hwDeviceWakeup pAdapter->Previous_WibroStatus =%d!!", pAdapter->Previous_WibroStatus);

		// if modem is already waked up, send UpNoti to WCM
		if(gpio_get_value(GPIO_WIMAX_CON0) && (pAdapter->Previous_WibroStatus == WIBRO_STATE_IDLE))
		{
			nlength = ModemUpNotiMsg(buf);
			controlReceive(pAdapter, buf, nlength);
		}
		// modem is dead, send RESET command to WCM
		else if(!gpio_get_value(GPIO_WIMAX_CON0))
		{
			nlength= ModemResetMsg(buf);
			controlReceive(pAdapter, buf, nlength);
		}
	}

	if(pAdapter->WibroStatus == WIBRO_STATE_AWAKE_REQUESTED)
	{
		if(pAdapter->Previous_WibroStatus == WIBRO_STATE_IDLE){
			DumpDebug(HARDWARE, "hwDeviceWakeup: IDLE -> NORMAL");
			pAdapter->WibroStatus = WIBRO_STATE_NORMAL;
		}
		else if(pAdapter->Previous_WibroStatus == WIBRO_STATE_VIRTUAL_IDLE){
			DumpDebug(HARDWARE, "hwDeviceWakeup: -> READY");
			pAdapter->WibroStatus = WIBRO_STATE_READY;
		}
		else
		{
			DumpDebug(HARDWARE, "hwDeviceWakeup: in abnormal case in status %d", pAdapter->Previous_WibroStatus);
			pAdapter->WibroStatus =  pAdapter->Previous_WibroStatus;
		}
	}
	return 0;	
}

#else
INT	hwDeviceWakeup(PMINIPORT_ADAPTER pAdapter)
{
	CHAR str[] = "WAKE";
	UCHAR	nCount = 0;
	int nRet = 0;
	
	ENTER;
	do {
		nRet = sdio_memcpy_toio(pAdapter->func, SDIO_DATA_PORT_REG, str, 4);	
		nCount++;
		DumpDebug(HARDWARE, "device wakeup fail..");
	}while((nRet) && (!pAdapter->bHaltPending) && (nCount < HARDWARE_WAKE_MAX_COUNTER) );

	if(nRet) {
		DumpDebug(HARDWARE, "Retry wake-up sequence");
		mdelay(HARDWARE_WAKEUP_TIMEOUT);
	}
	LEAVE;
	return nRet;
}
#endif

void hwTransmitThread(struct work_struct *work)
{	
	MINIPORT_ADAPTER *pAdapter = container_of(work, struct MINIPORT_ADAPTER, work);
	PBUFFER_DESCRIPTOR dsc;
	HW_PRIVATE_PACKET hdr;
	int nRet = 0;
	ENTER;

	//sdio_claim_host(pAdapter->func);

#ifdef WAKEUP_BY_GPIO
	if (pAdapter->WibroStatus == WIBRO_STATE_IDLE ||pAdapter->WibroStatus == WIBRO_STATE_VIRTUAL_IDLE || !gpio_get_value(GPIO_WIMAX_CON0) )				
#else
	if(pAdapter->WibroStatus == WIBRO_STATE_VIRTUAL_IDLE || pAdapter->WibroStatus == WIBRO_STATE_IDLE)
#endif		
	{
		DumpDebug(TX_CONTROL, "hwTransmitThread : Try to Wakeup");
		hwDeviceWakeup(pAdapter);
	}

//	spin_lock_irqsave(&Adapter->SendLock,flags); 
	while( !QueueEmpty(pAdapter->hw.Q_Send.Head)) 
	{
		if(pAdapter->bHaltPending) {
			// send stop message
			hdr.Id0	 = 'W';
			hdr.Id1	 = 'P';
			hdr.Code  = HwCodeHaltedIndication;
			hdr.Value = 0;
			
			if( sdio_memcpy_toio(pAdapter->func,SDIO_DATA_PORT_REG, &hdr, sizeof(HW_PRIVATE_PACKET)) )
				DumpDebug(RX_DPC, "bHaltPending, send HaltIndication to FW err");
			break;
		}

		dsc = (PBUFFER_DESCRIPTOR) QueueGetHead(pAdapter->hw.Q_Send.Head);
		if (!dsc)
		{
			DumpDebug(DISPATCH, "Fail...node is null");
			break ;
		}

		sdio_claim_host(pAdapter->func);
		nRet = SendDataOut(pAdapter, dsc);
		sdio_release_host(pAdapter->func);
		if(nRet == STATUS_SUCCESS) {
			//DumpDebug(DISPATCH, "Send Packet Success");
			QueueRemoveHead(pAdapter->hw.Q_Send.Head);
			if(dsc->Buffer)
				kfree(dsc->Buffer);
			if(dsc)	
				kfree(dsc);
		}
		else {
			DumpDebug(DISPATCH,("SendData Fail******"));
			++pAdapter->XmitErr;
			if(nRet == 	-ENOMEDIUM || nRet == /*-ETIMEOUT*/-110 ) {	//cky 20100217 add for modem die
				pAdapter->bHaltPending = TRUE;
				break;
			}
		}
	}
//	spin_unlock_irqrestore(&Adapter->SendLock,flags);
//	sdio_release_host(pAdapter->func);
	LEAVE;
	return ;
}

