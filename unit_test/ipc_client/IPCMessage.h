#pragma once

#define IPC_MESSAGE_START TestMsgStart

// Generic message class that is an int followed by a wstring.
IPC_MESSAGE_CONTROL2(MsgClassIS, int, std::wstring)

// Generic message class that is a wstring followed by an int.
IPC_MESSAGE_CONTROL2(MsgClassSI, std::wstring, int)

// Used to generate an ID for a message that should not exist.
IPC_MESSAGE_CONTROL0(MsgUnhandled)