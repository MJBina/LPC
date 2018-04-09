/**********************************************************************
* © 2005 Microchip Technology Inc.
*
* FileName:        DataEEPROM.s
* Dependencies:    Header (*.inc/.h) files if applicable, see below
* Processor:       dsPIC30Fxxxx
* Compiler:        MPLAB® C30 v3.00 or higher
* IDE:             MPLAB® IDE v7.52 or later
* Dev. Board Used: dsPICDEM 1.1 Development Board
* Hardware Dependencies: None
*
* SOFTWARE LICENSE AGREEMENT:
* Microchip Technology Incorporated ("Microchip") retains all ownership and 
* intellectual property rights in the code accompanying this message and in all 
* derivatives hereto.  You may use this code, and any derivatives created by 
* any person or entity by or on your behalf, exclusively with Microchip's 
* proprietary products.  Your acceptance and/or use of this code constitutes 
* agreement to the terms and conditions of this notice.
*
* CODE ACCOMPANYING THIS MESSAGE IS SUPPLIED BY MICROCHIP "AS IS".  NO 
* WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT NOT LIMITED 
* TO, IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS FOR A 
* PARTICULAR PURPOSE APPLY TO THIS CODE, ITS INTERACTION WITH MICROCHIP'S 
* PRODUCTS, COMBINATION WITH ANY OTHER PRODUCTS, OR USE IN ANY APPLICATION. 
*
* YOU ACKNOWLEDGE AND AGREE THAT, IN NO EVENT, SHALL MICROCHIP BE LIABLE, WHETHER 
* IN CONTRACT, WARRANTY, TORT (INCLUDING NEGLIGENCE OR BREACH OF STATUTORY DUTY), 
* STRICT LIABILITY, INDEMNITY, CONTRIBUTION, OR OTHERWISE, FOR ANY INDIRECT, SPECIAL, 
* PUNITIVE, EXEMPLARY, INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, FOR COST OR EXPENSE OF 
* ANY KIND WHATSOEVER RELATED TO THE CODE, HOWSOEVER CAUSED, EVEN IF MICROCHIP HAS BEEN 
* ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE.  TO THE FULLEST EXTENT 
* ALLOWABLE BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN ANY WAY RELATED TO 
* THIS CODE, SHALL NOT EXCEED THE PRICE YOU PAID DIRECTLY TO MICROCHIP SPECIFICALLY TO 
* HAVE THIS CODE DEVELOPED.
*
* You agree that you are solely responsible for testing the code and 
* determining its suitability.  Microchip has no obligation to modify, test, 
* certify, or support the code.
*
* REVISION HISTORY:
*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
* Author         Date      Comments on this revision
*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
* EB/HV          11/02/05  First release of source file
*
*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
* ADDITIONAL NOTES:
**********************************************************************/

.include "p30fxxxx.inc"

.equ    EE_WORD_ERASE_CODE, 0x4044
.equ    EE_WORD_WRITE_CODE, 0x4004
.equ    EE_ROW_ERASE_CODE, 0x4045
.equ    EE_ROW_WRITE_CODE, 0x4005
.equ    EE_ALL_ERASE_CODE, 0x4046
.equ    CONFIG_WORD_WRITE_CODE, 0x4006

.global _ReadEE
.global _EraseEE
.global _WriteEE

.section .text

;------------------------------------------------------------------------------
;	int ReadEE(int Page, int Offset, int* DataOut, int Size);
;
;	Parameters:
;	(W0)  Page:        8 MS-bits of the source address in EEPROM
;	(W1)  Offset:      16 LS-bits of the source address in EEPROM
;	(W2)  DataOut:     16-bit address of the destination RAM location or array
;	(W3)  Size:        number of words to read: 1 or 16
;
;	Returns:		
;		ERROREE (or -1) if Size is invalid
;==============================================================================
 
_ReadEE:
        push    TBLPAG			
        mov     w0, TBLPAG
        cp      w3, #1			
        bra     z, L0			; branch if reading just one word
        cp      w3, #16			; else, 
        bra     z, L0			; branch if reading 16-words
        mov     #-1, w0			; else, error; load -1 into W0 and return
        bra     L1
L0:     tblrdl  [w1++],[w2++]	; TBLRDL; Read Low Program Word into W2
        dec     w3, w3			; W3 = W3 - 1
        bra     nz, L0			; REPEAT until W3 <= 0;
								; When we are done, we return something the 
								; passed 'Page' value... it is not -1 ???
L1:     pop     TBLPAG			
        return

;------------------------------------------------------------------------------
;	int EraseEE(int Page, int Offset, int Size);

;	Parameters:
;	(W0)  Page:        8 MS-bits of the address to be erased
;	(W1)  Offset:      16 LS-bits of the address to be erased
;	(W2)  Size:        Number of words to erase; 1, 16 or 0xFFFF (Erase ALL)
;
;	Returns:		
;		ERROREE (or -1) if Size is invalid
;==============================================================================

_EraseEE:
        push.d  w4				; Push Double
        bclr    SR, #Z			; Clear bit Z (Zero) in the ALU Status Register

        mov     #EE_WORD_ERASE_CODE, W4
        cp      w2, #1
        bra     z, L2			; branch if erasing one word
        mov     #EE_ROW_ERASE_CODE, W4
        cp      w2, #16
        bra     z, L2			; branch if erasing 16-words
        mov     #EE_ALL_ERASE_CODE, W4
        mov     #0xFFFF, w5
        cp      w2, w5
        bra     z, L2			; branch if erasing ALL EEPROM
        mov     #-1, w0			; else, error; load -1 into W0 and return
        pop.d   w4
        return
L2:
        push    TBLPAG
        mov     W0, NVMADRU
        mov     W1, NVMADR
        mov     W4, NVMCON
        push    SR				; Save ALU & DSP the status register
        
		mov     #0xE0, W0		; DISABLE (USER) INTERRUPTS
        ior     SR				; Logical OR low-byte of Status Register to set
								; IPL (Interrupt Priority Level) bits
        mov     #0x55, W0
        mov     W0, NVMKEY
        mov     #0xAA, W0
        mov     W0, NVMKEY
        bset    NVMCON, #WR
        nop
        nop
L3:     btsc    NVMCON, #WR
        bra     L3
        clr     w0
        pop     SR				; Restore the ALU & DSP the status registers
								; RE-ENABLE (USER) INTERRUPTS
L4:     pop     TBLPAG
        pop.d   w4
        return


/* DATA EEPROM Write Routines */
_WriteEE:
        push    w4
        bclr    SR, #Z
        mov     #EE_WORD_WRITE_CODE, W4
        cp      w3, #1
        bra     z, L5
        mov     #EE_ROW_WRITE_CODE, W4
        cp      w3, #16
        bra     z, L5
        pop     w4
        mov     #-1, w0
        return

L5:     push    TBLPAG
        mov     W1, TBLPAG
        push    W2
L6:     tblwtl  [W0++],[W2++]
        dec     w3, w3
        bra     nz, L6

        mov     W1, NVMADRU
        pop     W2
        mov     W2, NVMADR
        mov     W4, NVMCON
        push    SR
        mov     #0xE0, W0
        ior     SR
        mov     #0x55, W0
        mov     W0, NVMKEY
        mov     #0xAA, W0
        mov     W0, NVMKEY
        bset    NVMCON, #WR
        nop
        nop
L7:     btsc    NVMCON, #WR
        bra     L7
        clr     w0
        pop     SR
        pop     TBLPAG
        pop     w4
        return


.end
