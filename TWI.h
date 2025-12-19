#ifndef TWI_IS_SET

#ifndef TWI_SDA
	/* Для ATmega328p */
	#define TWI_SDA PC4
#endif

#ifndef TWI_SCL
	/* Для ATmega328p */
	#define TWI_SCL PC5
#endif

#ifndef TWI_SDA_PORT
	/* Для ATmega328p */
	#define TWI_SDA_PORT PORTC
#endif

#ifndef TWI_SCL_PORT
	/* Для ATmega328p */
	#define TWI_SCL_PORT PORTC
#endif

/* !!! Нужно указать !!! */
#define TWI_N_CMDS 16

#ifndef TWI_N_CMDS
	#define TWI_N_CMDS 0
#endif

#define TWI_WRITE 0
#define TWI_READ 1

/* Ожидание ответа TWI */
extern void TWI_wait();

/* Сигнал (s) */
extern unsigned char TWI_start();

/* Подключение к slave-устройству */
extern unsigned char TWI_connect(unsigned char slave_address, 
		unsigned char mode);
		
/* Сигнал (p) */
extern unsigned char TWI_stop(unsigned char f_disable_TWI);

/* Передать 1 байт */
extern unsigned char TWI_write_byte(unsigned char byte);

/* Передать массив */
extern unsigned char TWI_write_buf(unsigned char *buf, unsigned char len);

/* Прочитать 1 байт, отправить (A) */
extern unsigned char TWI_read_byte_unsafe();

/* Прочитать 1 байт, отправить (N) 
 * (сигнал слейву, чтобы он перестал отправлять данные) */
extern unsigned char TWI_read_last_byte_unsafe();

extern void TWI_init_pullups();

/* Status=0 - все нормально */
extern unsigned char get_SLA_address(unsigned char* out_status);

/* Работа по прерываниям 
 * Глобальные переменные:
 * buf - буфер для очередного считывания
 * out_buf - защищенный выходной буфер с безопасным обновлением
 * buf_cntr - указатель текущей ячейки для записи в буфер
 * out_buf_len (== buf_cntr+1) - длина выходного буфера
 * f_read - флаги (упр. сигналы от упр. автомата)
 * f_read_first
 * f_read_last 
 * schedule - массив команд УА [(uchar) код команды, (uchar) парам]
 * TWI_N_CMDS - число команд 
 * BUF_LEN - длина буфера */

enum TWI_Code {
	TWI_CODE_S,
	TWI_CODE_WRITE,
	TWI_CODE_READ_FIRSTB,
	TWI_CODE_READ_B,
	TWI_CODE_READ_LASTB,
	TWI_CODE_READ_1_B,
	TWI_CODE_HALT
};

typedef struct {
	volatile unsigned char f_read;
	volatile unsigned char f_read_first;
	volatile unsigned char f_read_last;
} TWI_Control_Signals;

/*
typedef struct {
	unsigned char *cmds[2];
	const unsigned char n_cmds;
	volatile char ip;
} TWI_Int_Controller;
*/

extern void TWI_int_init_cycle(volatile char *TWI_cmd_cntr, 
		TWI_Control_Signals *signals);

extern void TWI_int_cycle(unsigned char schedule[][2], 
		volatile char *TWI_cmd_cntr,
		TWI_Control_Signals *signals);

extern void TWI_int_read_proc(volatile unsigned char *buf, 
		volatile unsigned char *out_buf,
		volatile char *buf_cntr,
		volatile char *out_buf_len,
		TWI_Control_Signals *signals);
		
#define TWI_IS_SET
#endif
