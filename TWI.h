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
unsigned char TWI_stop() {
	TWCR = (1 << TWSTO) | (1 << TWINT) | (1 << TWEN);
	TWI_wait();
	
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
