; Copyright 2021 Bruce Dawson
;
; Licensed under the Apache License, Version 2.0 (the "License");
; you may not use this file except in compliance with the License.
; You may obtain a copy of the License at
;
;     http://www.apache.org/licenses/LICENSE-2.0
;
; Unless required by applicable law or agreed to in writing, software
; distributed under the License is distributed on an "AS IS" BASIS,
; WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
; See the License for the specific language governing permissions and
; limitations under the License.

.CODE

; Spin in a loop for 50*spinCount cycles with an IPC of ~1.0.
; On out-of-order CPUs the sub and jne will not add
; any execution time.

SpinALot1 PROC ; (const int spinCount : rcx)
		; spinCount is already in ecx
start:
		; Ten dependent adds
		add eax, eax
		add eax, eax
		add eax, eax
		add eax, eax
		add eax, eax

		add eax, eax
		add eax, eax
		add eax, eax
		add eax, eax
		add eax, eax

		; Ten more dependent adds
		add eax, eax
		add eax, eax
		add eax, eax
		add eax, eax
		add eax, eax

		add eax, eax
		add eax, eax
		add eax, eax
		add eax, eax
		add eax, eax

		; Ten more dependent adds
		add eax, eax
		add eax, eax
		add eax, eax
		add eax, eax
		add eax, eax

		add eax, eax
		add eax, eax
		add eax, eax
		add eax, eax
		add eax, eax

		; Ten more dependent adds
		add eax, eax
		add eax, eax
		add eax, eax
		add eax, eax
		add eax, eax

		add eax, eax
		add eax, eax
		add eax, eax
		add eax, eax
		add eax, eax

		; Ten more dependent adds
		add eax, eax
		add eax, eax
		add eax, eax
		add eax, eax
		add eax, eax

		add eax, eax
		add eax, eax
		add eax, eax
		add eax, eax
		add eax, eax


		sub ecx, 1
		jne start

        ret 0
SpinALot1 ENDP



; Spin in a loop for 50*spinCount cycles with an IPC of ~2.0.

SpinALot2 PROC ; (const int spinCount : rcx)
		; spinCount is already in ecx
start:
		; Twenty instructions that should take ten cycles.
		shl eax, 1
		add r8, r8
		shl eax, 1
		add r8, r8
		shl eax, 1
		add r8, r8
		shl eax, 1
		add r8, r8
		shl eax, 1
		add r8, r8

		shl eax, 1
		add r8, r8
		shl eax, 1
		add r8, r8
		shl eax, 1
		add r8, r8
		shl eax, 1
		add r8, r8
		shl eax, 1
		add r8, r8

		; Twenty more instructions in ten cycles
		shl eax, 1
		add r8, r8
		shl eax, 1
		add r8, r8
		shl eax, 1
		add r8, r8
		shl eax, 1
		add r8, r8
		shl eax, 1
		add r8, r8

		shl eax, 1
		add r8, r8
		shl eax, 1
		add r8, r8
		shl eax, 1
		add r8, r8
		shl eax, 1
		add r8, r8
		shl eax, 1
		add r8, r8

		; Twenty more instructions in ten cycles
		shl eax, 1
		add r8, r8
		shl eax, 1
		add r8, r8
		shl eax, 1
		add r8, r8
		shl eax, 1
		add r8, r8
		shl eax, 1
		add r8, r8

		shl eax, 1
		add r8, r8
		shl eax, 1
		add r8, r8
		shl eax, 1
		add r8, r8
		shl eax, 1
		add r8, r8
		shl eax, 1
		add r8, r8

		; Twenty more instructions in ten cycles
		shl eax, 1
		add r8, r8
		shl eax, 1
		add r8, r8
		shl eax, 1
		add r8, r8
		shl eax, 1
		add r8, r8
		shl eax, 1
		add r8, r8

		shl eax, 1
		add r8, r8
		shl eax, 1
		add r8, r8
		shl eax, 1
		add r8, r8
		shl eax, 1
		add r8, r8
		shl eax, 1
		add r8, r8

		; Twenty more instructions in ten cycles
		shl eax, 1
		add r8, r8
		shl eax, 1
		add r8, r8
		shl eax, 1
		add r8, r8
		shl eax, 1
		add r8, r8
		shl eax, 1
		add r8, r8

		shl eax, 1
		add r8, r8
		shl eax, 1
		add r8, r8
		shl eax, 1
		add r8, r8
		shl eax, 1
		add r8, r8
		shl eax, 1
		add r8, r8


		sub ecx, 1
		jne start

        ret 0
SpinALot2 ENDP



; Spin in a loop for 50*spinCount cycles with an IPC of ~3.0.

SpinALot3 PROC ; (const int spinCount : rcx)
		; spinCount is already in ecx
start:
		; Thirty instructions that should take ten cycles.
		; Instruction mix suggested here: https://twitter.com/komrad36/status/1471206968765284354
		; Three independent add instructions only get an IPC of 2.4, apparently due to suboptimal
		; execution port scheduling.
		; Three independent shift instructions only get an IPC of 2.0. So, a mix it is. This gives
		; us an IPC of 2.85.
		shl eax, 1
		add r8, r8
		shl edx, 1
		shl eax, 1
		add r8, r8
		shl edx, 1
		shl eax, 1
		add r8, r8
		shl edx, 1
		shl eax, 1
		add r8, r8
		shl edx, 1
		shl eax, 1
		add r8, r8
		shl edx, 1

		shl eax, 1
		add r8, r8
		shl edx, 1
		shl eax, 1
		add r8, r8
		shl edx, 1
		shl eax, 1
		add r8, r8
		shl edx, 1
		shl eax, 1
		add r8, r8
		shl edx, 1
		shl eax, 1
		add r8, r8
		shl edx, 1

		; Thirty more instructions in ten cycles
		shl eax, 1
		add r8, r8
		shl edx, 1
		shl eax, 1
		add r8, r8
		shl edx, 1
		shl eax, 1
		add r8, r8
		shl edx, 1
		shl eax, 1
		add r8, r8
		shl edx, 1
		shl eax, 1
		add r8, r8
		shl edx, 1

		shl eax, 1
		add r8, r8
		shl edx, 1
		shl eax, 1
		add r8, r8
		shl edx, 1
		shl eax, 1
		add r8, r8
		shl edx, 1
		shl eax, 1
		add r8, r8
		shl edx, 1
		shl eax, 1
		add r8, r8
		shl edx, 1

		; Thirty more instructions in ten cycles
		shl eax, 1
		add r8, r8
		shl edx, 1
		shl eax, 1
		add r8, r8
		shl edx, 1
		shl eax, 1
		add r8, r8
		shl edx, 1
		shl eax, 1
		add r8, r8
		shl edx, 1
		shl eax, 1
		add r8, r8
		shl edx, 1

		shl eax, 1
		add r8, r8
		shl edx, 1
		shl eax, 1
		add r8, r8
		shl edx, 1
		shl eax, 1
		add r8, r8
		shl edx, 1
		shl eax, 1
		add r8, r8
		shl edx, 1
		shl eax, 1
		add r8, r8
		shl edx, 1

		; Thirty more instructions in ten cycles
		shl eax, 1
		add r8, r8
		shl edx, 1
		shl eax, 1
		add r8, r8
		shl edx, 1
		shl eax, 1
		add r8, r8
		shl edx, 1
		shl eax, 1
		add r8, r8
		shl edx, 1
		shl eax, 1
		add r8, r8
		shl edx, 1

		shl eax, 1
		add r8, r8
		shl edx, 1
		shl eax, 1
		add r8, r8
		shl edx, 1
		shl eax, 1
		add r8, r8
		shl edx, 1
		shl eax, 1
		add r8, r8
		shl edx, 1
		shl eax, 1
		add r8, r8
		shl edx, 1

		; Thirty more instructions in ten cycles
		shl eax, 1
		add r8, r8
		shl edx, 1
		shl eax, 1
		add r8, r8
		shl edx, 1
		shl eax, 1
		add r8, r8
		shl edx, 1
		shl eax, 1
		add r8, r8
		shl edx, 1
		shl eax, 1
		add r8, r8
		shl edx, 1

		shl eax, 1
		add r8, r8
		shl edx, 1
		shl eax, 1
		add r8, r8
		shl edx, 1
		shl eax, 1
		add r8, r8
		shl edx, 1
		shl eax, 1
		add r8, r8
		shl edx, 1
		shl eax, 1
		add r8, r8
		shl edx, 1


		sub ecx, 1
		jne start

        ret 0
SpinALot3 ENDP

END
