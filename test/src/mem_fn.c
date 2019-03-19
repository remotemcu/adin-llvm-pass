typedef union {
  struct {
    int :1;               /*!< bit:      0  Reserved                           */
    int ENABLE:1;         /*!< bit:      1  Enable                             */
    int :4;               /*!< bit:  2.. 5  Reserved                           */
    int RUNSTDBY:1;       /*!< bit:      6  Run during Standby                 */
    int ONDEMAND:1;       /*!< bit:      7  Enable on Demand                   */
    int PRESC:2;          /*!< bit:  8.. 9  Prescaler Select                   */
    int :6;               /*!< bit: 10..15  Reserved                           */
    int CALIB:12;         /*!< bit: 16..27  Calibration Value                  */
    int :2;               /*!< bit: 28..29  Reserved                           */
    int FRANGE:2;         /*!< bit: 30..31  Frequency Range                    */
  } bit;                       /*!< Structure used for bit  access                  */
  int reg;                /*!< Type      used for register access              */
} SYSCTRL_OSC8M_Type;

typedef struct {
       char                    Reserved3[0x3];
  SYSCTRL_OSC8M_Type        OSC8M;       /**< \brief Offset: 0x20 (R/W 32) OSC8M Control A */

} Sysctrl;



void q(char a[])
{

  char arr[11];
  SYSCTRL_OSC8M_Type temp = ((Sysctrl*)0)->OSC8M;

	((Sysctrl*)0)->OSC8M = temp;

  memset(a, 0, 11);
  memmove(arr, a, 11);

}