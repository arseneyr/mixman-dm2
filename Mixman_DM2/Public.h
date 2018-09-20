/*++

Module Name:

    public.h

Abstract:

    This module contains the common declarations shared by driver
    and user applications.

Environment:

    user and kernel

--*/

#ifndef PUBLIC_H
#define PUBLIC_H

//
// Define an Interface Guid so that app can find the device and talk to it.
//

DEFINE_GUID (GUID_DEVINTERFACE_MixmanDM2,
    0x2d83f9e0,0x703e,0x49fa,0xb2,0xab,0x2b,0xbc,0x98,0x5a,0x94,0x30);
// {2d83f9e0-703e-49fa-b2ab-2bbc985a9430}

#endif