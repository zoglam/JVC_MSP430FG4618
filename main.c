#include <msp430fg4618.h>

unsigned short counter;
unsigned short sendInfo; //Последовательность восьми битов

// Длительность отрезков JVC
#define HOLE_0 (0x20C) // Пауза для "0" - 524
#define HOLE_1 (0x626) // Пауза для "1" - 1574
#define SIGNAL (0x20E) // Длительность выдаваемого сигнала (горка) - 526

// Кнопка
#define SW1 (BIT0) // P1.0

int main(void)
{
	WDTCTL = WDTPW + WDTHOLD; // Отключение сторожевого
	FLL_CTL0 |= XCAP14PF;	 // Задаем конденсаторы для кварца

	P1IFG &= ~SW1, P1IES &= ~SW1, P1IE |= SW1;
	P3SEL |= BIT4; // P3.4 to TB3

	TBCTL = MC_1 + TASSEL_2; // UP + SMCLK(1048576 Hz)
	TBCCTL3 = OUTMOD_7;		 // Режим сравнения
	TBCCR0 = 0x1A;			 // (SMCLK/38 kHz) = 26
	TBCCR3 = 0x08;			 // 26/3 = 8 (1/3 duty)

	__bis_SR_register(LPM0_bits + GIE); // LPM0 + GlobalInterruptEnable
}

#pragma vector = PORT1_VECTOR // Port1 interrupt - SW1
__interrupt void S1BNT(void)
{
	unsigned char f, pIN;
	f = P1IFG, pIN = P1IN;
	sendInfo = 0x81; //Задать последовательность восьми битов

	if (f & SW1)
	{
		if (!(pIN & SW1))
		{
			TACCR0 = SIGNAL;
			counter = 0x00;
			P3DIR |= BIT4; // OUT -> P3.4
		}
		P1IFG &= ~SW1, P1IES ^= SW1;
	}
	// ON -> Taimer A
	TACCTL0 = CCIE;
	TACTL = MC_1 + TASSEL_2; // UP + SMCLK(1048576 Hz)
}

#pragma vector = TIMER0_A0_VECTOR // timerA0 interrupt
__interrupt void myTimerISR(void)
{
	if (!(TACCTL0 & CCI))
	{
		if (counter < 0x11 && counter % 0x02 == 0x00) // Отрезки с паузами
		{
			P3DIR &= ~BIT4; // IN -> P3.4
			counter++;
			TACCR0 = sendInfo & 0x80 ? HOLE_1 : HOLE_0;
			sendInfo = sendInfo << 1;
		}
		else if (counter < 0x11) // Отрезки с повторной подачей сигнала
		{
			P3DIR |= BIT4; // OUT -> P3.4
			counter++;
			TACCR0 = SIGNAL;
		}
		else // иначе все выключить
		{
			P3DIR &= ~BIT4; // IN -> P3.4
			// OFF -> Taimer A
			TACTL = 0x00;
			TACCTL0 = 0x00;
		}
	}
}
