/********************************************************************** 
* © 2006 Microchip Technology Inc. 
* 
* FileName:        getErrLoc.s 
* Dependencies:    Header (.h/.inc) files if applicable, see below 
* Processor:       dsPIC33Fxxxx/PIC24Hxxxx 
* Compiler:        MPLAB® C30 v2.01.00 or higher 
* 
* SOFTWARE LICENSE AGREEMENT: 
* Microchip Technology Inc. (Microchip) licenses this software to you 
* solely for use with Microchip dsPIC® digital signal controller 
* products. The software is owned by Microchip and is protected under 
* applicable copyright laws.  All rights reserved. 
* 
* SOFTWARE IS PROVIDED AS IS.  MICROCHIP EXPRESSLY DISCLAIMS ANY 
* WARRANTY OF ANY KIND, WHETHER EXPRESS OR IMPLIED, INCLUDING BUT NOT 
* LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A 
* PARTICULAR PURPOSE, OR NON-INFRINGEMENT. IN NO EVENT SHALL MICROCHIP 
* BE LIABLE FOR ANY INCIDENTAL, SPECIAL, INDIRECT OR CONSEQUENTIAL 
* DAMAGES, LOST PROFITS OR LOST DATA, HARM TO YOUR EQUIPMENT, COST OF 
* PROCUREMENT OF SUBSTITUTE GOODS, TECHNOLOGY OR SERVICES, ANY CLAIMS 
* BY THIRD PARTIES (INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF), 
* ANY CLAIMS FOR INDEMNITY OR CONTRIBUTION, OR OTHER SIMILAR COSTS. 
* 
* REVISION HISTORY: 
*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ 
* Author            Date      Comments on this revision 
*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ 
* RK                03/22/06  First release of source file 
* 
*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ 
* 
* ADDITIONAL NOTES: 
* 
**********************************************************************/ 
 
	.global 	_getErrLoc 
 
	.section .text 
 
; Stack Growth from Trap Error 
;1. PC[15:0]            <--- Trap Address 
;2. SR[7:0]:IPL3:PC[22:16] 
;3. RCOUNT 
;4. W0 
;5. W1 
;6. W2 
;7. W3 
;8. W4 
;9. W5 
;10. W6 
;11. W7 
;12. OLD FRAME POINTER [W14] 
;13. PC[15:0]           <---- W14  
;14. 0:PC[22:16] 
;15.                    <---- W15 
 
_getErrLoc: 
        mov    w14,w2 
        sub    w2,#24,w2 
        mov    [w2++],w0 
        mov    [w2++],w1  
        mov    #0x7f,w3     ; Mask off non-address bits 
        and    w1,w3,w1 
 
        mov    #2,w2        ; Decrement the address by 2 
        sub    w0,w2,w0 
        clr    w2 
        subb   w1,w2,w1 
        return 
