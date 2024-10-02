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


#define TWSR_MASK 0b11111000
#define SHOW_DELAY 500
#define TWI_WRITE 0
#define TWI_READ 1
#define TWI_ACK 0
#define TWI_NACK 1

#define TWI_POWER_OFF 1
#define TWI_POWER_ON 1

/* Ожидание ответа TWI */
void TWI_wait() {
	while (!(TWCR & (1 << TWINT))); 
}

/* Сигнал (s) */
unsigned char TWI_start() {
	TWCR = 1 << TWEN;
	TWCR = (1 << TWSTA) | (1 << TWINT) | (1 << TWEN);
	TWI_wait();
	//TWCR &= ~(1 << TWSTA); // Меняет статус TWI
	return TWSR & TWSR_MASK;
	
}

/* Подключение к slave-устройству */
unsigned char TWI_connect(unsigned char slave_address, 
		unsigned char mode) {
	/* Получатся, TWSTA должен и здесь быть == 1 */
	TWDR = (slave_address * 2) + mode;
	TWCR = (1 << TWINT) | (1 << TWEN);
	TWI_wait();
	return TWSR & TWSR_MASK;
}

/* (p) */
unsigned char TWI_stop(unsigned char f_disable_TWI) {
	TWCR = (1 << TWSTO) | (1 << TWINT) | (1 << TWEN);
	TWI_wait();
	
	if (f_disable_TWI==1)
		TWCR = 0; // На всякий случай очищаем TWCR
	return TWSR & TWSR_MASK;
}

/* Передать 1 байт */
unsigned char TWI_write_byte(unsigned char byte) {
	TWDR = byte;
	//show_byte(TWCR);
	//TWCR |= 1 << TWINT;
	TWCR = (1 << TWINT) | (1 << TWEN); // Равенство решило проблему
	// Видимо TWSTA==1?
	TWI_wait();
	return TWSR & TWSR_MASK;
}

/* Прочитать 1 байт, отправить (A) */
unsigned char TWI_read_byte_unsafe() {
	TWCR = (1 << TWEN) | (1 << TWINT) | (1 << TWEA);
	TWI_wait();
	return TWDR;
}

/* Прочитать 1 байт, отправить (N) 
 * (сигнал слейву, чтобы он перестал отправлять данные) */
unsigned char TWI_read_last_byte_unsafe() {
	TWCR = (1 << TWEN) | (1 << TWINT);
	TWI_wait();
	return TWDR;
}

void TWI_init_pullups() {
	/* Подтяжка SDA и SCL к 1 */
	TWI_SCL_PORT |= 1 << TWI_SCL;
	TWI_SDA_PORT |= 1 << TWI_SDA;
}

/* Status=0 - все нормально */
unsigned char get_SLA_address(unsigned char* out_status) {
	volatile unsigned char sla=0, status=0x00;
	while (sla<128) {
		TWI_start();
		status = TWI_connect(sla, 0);
		if (status==0x18) {
			//show_byte(sla);
			break;
		}
		sla++;
	}
	*out_status=status;
	return sla;
}

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

#define TWI_SCHED_CMD 0
#define TWI_SCHED_PARAM 1

typedef struct {
	volatile unsigned char f_read;
	volatile unsigned char f_read_first;
	volatile unsigned char f_read_last;
} TWI_Control_Signals;
typedef struct {

	unsigned char *cmds[2];
	const unsigned char n_cmds;
	volatile char ip;
} TWI_Int_Controller;

void TWI_int_init_cycle(volatile char *TWI_cmd_cntr, 
		TWI_Control_Signals *signals, 
		unsigned char sla) {
	*TWI_cmd_cntr = -1;
	signals->f_read=0;
	signals->f_read_first=0;
	signals->f_read_last=0;
	//TWI_start();
	//TWI_connect(sla,0);
	TWCR = (1 << TWEN) | (1 << TWINT) | (1 << TWIE); 
}

void TWI_int_cycle(unsigned char schedule[][2], 
		volatile char *TWI_cmd_cntr,
		TWI_Control_Signals *signals) {
	
	*TWI_cmd_cntr = (*TWI_cmd_cntr)+1;
	if ((*TWI_cmd_cntr) == TWI_N_CMDS) { 
		//show_byte(0xF0);
		(*TWI_cmd_cntr)=0;
	}
		
	if (schedule[*TWI_cmd_cntr][TWI_SCHED_CMD]==TWI_CODE_S) {
		TWCR = (1 << TWSTA) | (1 << TWINT) | (1 << TWEN) | (1 << TWIE);
	}
	
	if (schedule[*TWI_cmd_cntr][TWI_SCHED_CMD]==TWI_CODE_WRITE) {
		//show_byte(schedule[*TWI_cmd_cntr][TWI_SCHED_PARAM]);
		TWDR = schedule[*TWI_cmd_cntr][TWI_SCHED_PARAM];
		TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWIE);
	}
		
	if (schedule[*TWI_cmd_cntr][TWI_SCHED_CMD]==TWI_CODE_READ_FIRSTB) {
		signals->f_read=1;
		signals->f_read_first=1;
		TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWIE) | (1 << TWEA);
	}
	if (schedule[*TWI_cmd_cntr][TWI_SCHED_CMD]==TWI_CODE_READ_B) {
		signals->f_read=1;
		TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWIE) | (1 << TWEA);
	}
	if (schedule[*TWI_cmd_cntr][TWI_SCHED_CMD]==TWI_CODE_READ_LASTB) {
		signals->f_read=1;
		signals->f_read_last=1;
		//show_byte(*TWI_cmd_cntr);
		TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWIE);
	}
	if (schedule[*TWI_cmd_cntr][TWI_SCHED_CMD]==TWI_CODE_READ_1_B) {
		signals->f_read=1;
		signals->f_read_first=1;
		signals->f_read_last=1;
		//show_byte(*TWI_cmd_cntr);
		TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWIE);
	}
	/* TWI_HALT - ничего не происходит 
	 * и цикл останавливается (не происходит "обнуление" TWINT) */
	
}

void TWI_int_read_proc(volatile unsigned char *buf, 
		volatile unsigned char *out_buf,
		volatile char *buf_cntr,
		volatile char *out_buf_len,
		TWI_Control_Signals *signals) {
	
	unsigned char data = TWDR;
	
	if ((signals->f_read_first)==1) {
		*buf_cntr=-1;
		signals->f_read_first=0;
	}
	
	if ((signals->f_read)==1) {
		(*buf_cntr)=(*buf_cntr)+1;
		signals->f_read=0;
		buf[*buf_cntr]=data; //Может быть здесь успевает измениться -?
	}
	
	if ((signals->f_read_last)==1) {
		/* Critical section */
		cli();
		for (unsigned char i=0; i<=(*buf_cntr); i++)
			out_buf[i]=buf[i];
		(*out_buf_len)=(*buf_cntr)+1;
		
		/* Нужно записывать 0 в TWINT иначе будет бесконечный
		 * вызов обработчика (если дальше halt) */
		/* В руководстве рекомедуется не записывать 1 в TWIE */
		TWCR = (1 << TWEN) | (1 << TWINT);
		
		sei();
		/* End of critical section */
		signals->f_read_last=0;
	}
}
		
