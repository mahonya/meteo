#include <msp430.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

//work firmware
//send 1 packet of 5 minuts

int i,i0,i00,i1,n,n0,n1=0;
char strn[255],str0[255],str1[255];
unsigned int Wind,Direct;
unsigned int timer_dec=0;
char s;
int minut=0;
unsigned char up0,down0,up1,down1;
char END[]="\r\n";
char timer_off=0,port_off=0;
unsigned int Direct_Wind=0,Direct_Wind_timer=0;
unsigned int Wind=0,Wind_timer=0;


void SendUART0(char st){
      while (!(UCA0IFG&UCTXIFG));
      UCA0TXBUF=st;
}

void SendUART1(char st){
      while (!(UCA1IFG&UCTXIFG));
      UCA1TXBUF=st;
}

void UART0WriteString(char *str)
{
    int strSize = strlen(str);
  for(int i = 0; i<strSize; i++)
  {
    SendUART0(str[i]);
  }
}

void UART1WriteString(char *str)
{
    int strSize = strlen(str);
  for(int i = 0; i<strSize; i++)
  {
    SendUART1(str[i]);
  }
}



void Init12MHz(void){
  UCSCTL3 |= SELREF_2;                      // Set DCO FLL reference = REFO
  UCSCTL4 |= SELA_2;                        // Set ACLK = REFO

  __bis_SR_register(SCG0);                  // Disable the FLL control loop
  UCSCTL0 = 0x0000;                         // Set lowest possible DCOx, MODx
  UCSCTL1 = DCORSEL_5;                      // Select DCO range 24MHz operation
  UCSCTL2 = FLLD_1 + 374;                   // Set DCO Multiplier for 12MHz
                                            // (N + 1) * FLLRef = Fdco
                                            // (374 + 1) * 32768 = 12MHz
                                            // Set FLL Div = fDCOCLK/2
  __bic_SR_register(SCG0);                  // Enable the FLL control loop

  // Worst-case settling time for the DCO when the DCO range bits have been
  // changed is n x 32 x 32 x f_MCLK / f_FLL_reference. See UCS chapter in 5xx
  // UG for optimization.
  // 32 x 32 x 12 MHz / 32,768 Hz = 375000 = MCLK cycles for DCO to settle
  __delay_cycles(375000);
	
  // Loop until XT1,XT2 & DCO fault flag is cleared
  do
  {
    UCSCTL7 &= ~(XT2OFFG + XT1LFOFFG + DCOFFG);
                                            // Clear XT2,XT1,DCO fault flags
    SFRIFG1 &= ~OFIFG;                      // Clear fault flags
  }while (SFRIFG1&OFIFG);                   // Test oscillator fault flag
	
  
}
 
void InitUART(void){
  P3SEL |= BIT3+BIT4;                       // P3.3,4 = USCI_A0 TXD/RXD
  UCA0CTL1 |= UCSWRST;                      // **Put state machine in reset**
  UCA0CTL1 |= UCSSEL_2;                     // SMCLK
  UCA0BR0 = 0x68;                              // 12MHz 115200 (see User's Guide)
  //UCA0BR0 = 0x09;                              // 1MHz 115200 (see User's Guide)
  UCA0BR1 = 0;                              // 1MHz 115200
  UCA0MCTL |= UCBRS_1 + UCBRF_0;            // Modulation UCBRSx=1, UCBRFx=0
  UCA0CTL1 &= ~UCSWRST;                     // **Initialize USCI state machine**
  UCA0IE |= UCRXIE;                         // Enable USCI_A0 RX interrupt
   
  P4SEL |= BIT4+BIT5;
  UCA1CTL1 |= UCSWRST;                      // **Put state machine in reset**
  UCA1CTL1 |= UCSSEL_2;                     // SMCLK
  UCA1BR0 = 0x68;                              // 12MHz 115200 (see User's Guide)
  //UCA1BR0 = 0x09;                              // 1MHz 115200 (see User's Guide)
  UCA1BR1 = 0;                              // 1MHz 115200
  UCA1MCTL |= UCBRS_1 + UCBRF_0;            // Modulation UCBRSx=1, UCBRFx=0
  UCA1CTL1 &= ~UCSWRST;                     // **Initialize USCI state machine**
  UCA1IE |= UCRXIE;                         // Enable USCI_A0 RX interrupt
}

void SendHTTP(unsigned int Wind, unsigned int Direct){
  char wWWW[255];
   sprintf(wWWW, "wind=%d&direct=%d", Wind, Direct);
 UART0WriteString("AT"); 
 __delay_cycles(1200000);
 UART0WriteString("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\"\r\n"); 
 __delay_cycles(12000000);
 UART0WriteString("AT+SAPBR=3,1,\"APN\",\"internet\"\r\n"); 
 __delay_cycles(12000000);
 UART0WriteString("AT+SAPBR=3,1,\"USER\",\"gdata\"\r\n"); 
 __delay_cycles(1200000);
 UART0WriteString("AT+SAPBR=3,1,\"PWD\",\"gdata\"\r\n"); 
 __delay_cycles(1200000);
 UART0WriteString("AT+SAPBR=1,1\r\n"); 
 __delay_cycles(12000000);
 UART0WriteString("AT+HTTPINIT\r\n"); 
 __delay_cycles(12000000);
 UART0WriteString("AT+HTTPPARA=\"CID\",1\r\n");
 __delay_cycles(12000000);
 UART0WriteString("AT+HTTPPARA=\"URL\",\"http://mahon.ru/wind/index.php?");
 UART0WriteString(wWWW);
 UART0WriteString("\"\r\n");
 __delay_cycles(12000000);
 UART0WriteString("AT+HTTPACTION=0\r\n"); 
 __delay_cycles(12000000);
 UART0WriteString("AT+HTTPREAD\r\n"); 
 __delay_cycles(12000000);
 UART0WriteString("AT+HTTPTERM\r\n"); 

}

void InitADC(void){
  ADC12CTL0 = ADC12ON+ADC12MSC+ADC12SHT0_2; // Turn on ADC12, set sampling time
  ADC12CTL1 = ADC12SHP+ADC12CONSEQ_1;       // Use sampling timer, single sequence
  ADC12IE = 0x20;                           // Enable interrupt
  ADC12MCTL5 = ADC12INCH_5+ADC12EOS;        // ref+=AVcc, channel = A3, end seq.
  ADC12CTL0 |= ADC12ENC;
  P6SEL |= 0x10;                            // P6.0 ADC option select
  ADC12CTL0 |= ADC12SC;                     // Start convn - software trigger
}

void InitTimer(void){
  TA0CCTL0 = CCIE;                          // CCR0 interrupt enabled
  TA0CCTL1 = CCIE;                          // CCR1 interrupt enabled
  TA0CCR0 = 40960;                          //32768/8*10sec
  TA0CCR1 = 40960;
  TA0CTL = TASSEL_1 + MC_2 + TACLR + ID_3;  // SMCLK, upmode, clear TAR
}

void InitP2(void){
  P2REN |= BIT5;                            // Enable P1.4 internal resistance
  P2OUT |= BIT5;                            // Set P1.4 as pull-Up resistance
  P2IES |= BIT5;                            // P1.4 Hi/Lo edge
  P2IFG &= ~BIT5;                           // P1.4 IFG cleared
  P2IE |= BIT5;                             // P1.4 interrupt enabled
}

void InitP1(void){
  P4DIR |= BIT7|BIT1|BIT2;
}


int main(void)
{
  char wString[255];
  //char dString[5];
  WDTCTL = WDTPW + WDTHOLD;                 // Stop WDT
 
  Init12MHz();
  InitUART();
  InitP2();
  InitP1();
  InitADC();
  InitTimer();
  //start modem
  P4OUT |= BIT1;
  __delay_cycles(6000000);
  P4OUT &= ~BIT1;
   up0=0;
   down0=0;
   up1=0;
   down1=0;
 
  __bis_SR_register(GIE);       // Enter LPM0, interrupts enabled
  __no_operation();                         // For debugger
  
ADC12CTL0 |= ADC12SC;
  
//SendHTTP();
  
  
  while(1){
    while(up0!=down0){
      SendUART1(str0[++down0]);
    }
   
    while(up1!=down1){
      SendUART0(str1[++down1]);
    }
    
    if(minut>0){
      P4OUT |= BIT7;
      timer_dec=0;
      Wind=port_off;
      port_off=0;
      Direct=Direct_Wind;
      sprintf(wString,"Wind=%d Direct=%d\r\n",Wind,Direct);
      UART1WriteString(wString); 
      SendHTTP(Wind,Direct);  
      P4OUT &= ~BIT7; 
      minut=0;
    }
      __bis_SR_register(GIE+LPM1_bits); 
}
}


 
// Echo back RXed character, confirm TX buffer is ready first
#pragma vector=USCI_A0_VECTOR
__interrupt void USCI_A0_ISR(void)
{
  switch(__even_in_range(UCA0IV,4))
  {
  case 0:break;                             // Vector 0 - no interrupt
  case 2:                                   // Vector 2 - RXIFG
      str0[++up0]=UCA0RXBUF;
      LPM1_EXIT;
      break;
  case 4:
        //if(up1!=down1){
     //SendUART0(str1[++down1]);
    break;                             // Vector 4 - TXIFG
  default: break;
  }
}
 
#pragma vector=USCI_A1_VECTOR
__interrupt void USCI_A1_ISR(void)
{  
  switch(__even_in_range(UCA1IV,4))
  {
  case 0:break;                             // Vector 0 - no interrupt
  case 2:                                  
      str1[++up1]=UCA1RXBUF;
      LPM1_EXIT;
      break;
  case 4:break;                             // Vector 4 - TXIFG
  default: break;
  }
}

#pragma vector = ADC12_VECTOR
__interrupt void ADC12_ISR(void)
{

  switch(__even_in_range(ADC12IV,34))
  {
  case  0: break;                           // Vector  0:  No interrupt
  case  2: break;                           // Vector  2:  ADC overflow
  case  4: break;                           // Vector  4:  ADC timing overflow
  case  6: break;                                 // Vector  6:  ADC12IFG0
  case  8: break;                           // Vector  8:  ADC12IFG1
  case 10: break;                           // Vector 10:  ADC12IFG2
  case 12: break;                           // Vector 12:  ADC12IFG3
  case 14: break;                           // Vector 14:  ADC12IFG4
  case 16: // Vector 16:  ADC12IFG5
      //Direct_Wind=Direct_Wind/2+(ADC12MEM5-Direct_Wind_timer)/2;
      //Direct_Wind_timer=ADC12MEM5;
    Direct_Wind=ADC12MEM5;
    break;                           
  case 18: break;                           // Vector 18:  ADC12IFG6
  case 20: break;                           // Vector 20:  ADC12IFG7
  case 22: break;                           // Vector 22:  ADC12IFG8
  case 24: break;                           // Vector 24:  ADC12IFG9
  case 26: break;                           // Vector 26:  ADC12IFG10
  case 28: break;                           // Vector 28:  ADC12IFG11
  case 30: break;                           // Vector 30:  ADC12IFG12
  case 32: break;                           // Vector 32:  ADC12IFG13
  case 34: break;                           // Vector 34:  ADC12IFG14
  default: break; 
  }
}


// Timer0 A0 interrupt service routine
#pragma vector=TIMER0_A0_VECTOR
__interrupt void TIMER0_A0_ISR(void)
{
  //timer_off=1;
  //TA0CCR0 += 40960;
  //ADC12CTL0 |= ADC12SC; 
}

// Timer0_A5 Interrupt Vector (TAIV) handler
#pragma vector=TIMER0_A1_VECTOR
__interrupt void TIMER0_A1_ISR(void)
{
  switch(__even_in_range(TA0IV,14))
  {
    case  0: break;                          // No interrupt
    case  2: {
      TA0CCR1 += 40960;
      //P1OUT |= BIT0;
      timer_dec++;
      //P1OUT &= ~BIT0;
      if(timer_dec>5){
        minut++;
        LPM1_EXIT;
        //__bic_SR_register_on_exit(LPM0_bits);
        //__bic_SR_register(LPM0_bits);  
      }
      break;}                          // CCR1 not used
    case  4: break;                          // CCR2 not used
    case  6: break;                          // reserved
    case  8: break;                          // reserved
    case 10: break;                          // reserved
    case 12: break;                          // reserved
    case 14: break;                  // overflow
             
    default: break; 
  }
}


// Port 2 interrupt service routine
#pragma vector=PORT2_VECTOR
__interrupt void Port_2(void)
{

  TA0CCR0 += 40960;
  ADC12CTL0 |= ADC12SC;
  //Wind=Wind/2+(TA0R-Wind_timer)/2;
  //Wind_timer=TA0R;
  //Wind=TA0R;
  //Wind++;
  port_off++;
  //TA0CTL |= TACLR;
  P2IFG &= ~BIT5;                          // P1.4 IFG cleared
}