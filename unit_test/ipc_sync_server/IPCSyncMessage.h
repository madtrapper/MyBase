#ifndef __IPC_SYNC_MESSAGE__
#define __IPC_SYNC_MESSAGE__

#define IPC_MESSAGE_START TestMsgStart

// in1 must be false, out1 is true, out2 is 12
IPC_SYNC_MESSAGE_CONTROL1_2(Msg_C_1_2, bool, bool, int)


#endif