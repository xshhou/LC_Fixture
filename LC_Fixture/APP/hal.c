/*
 * hal.c
 *
 *  Created on: 2016��6��2��
 *      Author: wangjiuling
 */

#include "hal.h"
#include "adc.h"
#include "can.h"
#include "delay.h"
#include "flash.h"
#include "uart.h"
#include "timer.h"
#include "gpio.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "tool.h"

struct _uart		uart_pc;
struct _uart		uart_dut;
struct _adc_val*	adc_val;
struct _adc			adc;
struct _timer		timer2;
struct _timer		timer4;
struct _obj	cmd_to_dut;
struct _obj	reply_to_pc;
struct _obj	power_on_delay;
struct _obj	can;
struct _obj	motor;
struct _obj	hall;
struct _obj	temp;
struct _obj	exti_motor;
float ad_filter[M];
u8 cmd_list_len;
CanRxMsg can_rxbuf;
u8 can_data[] = {0,1,2,3,4,5,6,7};

extern volatile u16 ad_value[N][M];

struct _cmd_list cmd_list[] = {
	{"v5", test_v5},
	{"v3d3", test_v3d3},
	{"v24", test_v24},
	{"v6", test_v6},
	{"v12", test_v12},
	{"pwr_on", test_pwr_on},
	{"pwr_off", test_pwr_off},
	{"LCcurrent", test_current},
	{"barcode", test_barcode},
	{"can", test_can},
	{"motor", test_motor},
	{"hall", test_hall},
	{"temp", test_temp},
	{"force_pwr_on", force_pwr_on},
};

/**
 * @brief initiate hardware driver
 * 
 * @param  None
 * @retval None
 */
void hal_init()
{
	cmd_list_len = sizeof(cmd_list) / sizeof(struct _cmd_list);
	timer2.stop = timer2_stop;
	timer2.start = timer2_start;
	timer2.clear = timer2_clear;
	timer4.stop = timer4_stop;
	timer4.start = timer4_start;
	timer4.clear = timer4_clear;

	delay_init();
	adc_init();
	can_init();
	gpio_init();
	uart1_init();
	uart3_init();
	timer2_init(7200, 500);//10KHz, 50mS, DUT���ڷ������ݼ�ʱ��
	timer3_init(7200, 20);// 10KHz, 2mS, ���ڽ�����
	timer4_init(7200, 1000);// 10KHz, 100mS, CAN��ʱ��
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_CRC, ENABLE);
}

/**
 * @brief if find command in command buffer, then execute command
 *
 * @param buf command buffer
 * @param len length of command buffer
 * 
 * @retval None
 */
static int execute_cmd(const char* buf, u8 len)
{
	u8 i;
	u8 pos;
	char* cmd = NULL;
	char* tmp = NULL;
	char param[11] = {0};

	/* ���Ҵ����������Ŀǰֻ��һ���·������������ */
	tmp = strchr(buf, ':');
	if(tmp != NULL){
		pos = tmp - buf;
		cmd = (char*)malloc(pos + 1);
		*(cmd + pos) = '\0';
		memcpy(cmd, buf, pos);
		memcpy(param, tmp + 1, 10);
		param[10] = '\0';

		for(i = 0;i < cmd_list_len;i++){
			if(strcmp(cmd_list[i].str, cmd) == 0){
				cmd_list[i].cmd(param);
				break;
			}
		}
		free(cmd);
	}else{
		for(i = 0;i < cmd_list_len;i++){
			if(strcmp(cmd_list[i].str, buf) == 0){
				cmd_list[i].cmd(NULL);
				break;
			}
		}
	}

	return 0;
}
/**
 * @brief У���PC�յ��������Ƿ���ȷ
 *
 * @param  None
 * @retval None
 */
void verify_pc_data()
{
	/* ��ʱ����ʱ������ʱʱ����û�յ����ݣ���ʾһ֡���ݽ������ */
	if(uart_pc.timer.state == TIME_OUT){
		uart_pc.timer.state = NORMAL;
		/* У������ */
		if(verify_data(uart_pc.recv_buf, uart_pc.recv_len) == 1){
			/* ����У����ȷ����λ��־λ������handle_pc_data()���� */
			uart_pc.state = RECEIVED;
		}else{
			/* ��ϴ���ջ��� */
//			memset(uart_pc.recv_buf, 0, uart_pc.recv_len);
			uart_pc.recv_len = 0;
		}
	}
}
/**
 * @brief ִ�н��յ�������
 *
 * @param  None
 * @retval None
 */
void handle_pc_data()
{
	u8 len;

	if(uart_pc.state == RECEIVED){
		/* ȥ��ͷ(1)��β(1)��У���(2) */
		len = uart_pc.recv_len - 4;
		/* �����ڴ� *///TODO �����Ż��ɲ������ڴ�
		uart_pc.cmd = NULL;
		uart_pc.cmd = (char*)malloc(len + 1);
		if(uart_pc.cmd == NULL){
			return;
		}
		/* ������Ч���ݵ�uart_pc.cmd */
		memcpy(uart_pc.cmd, uart_pc.recv_buf + 1, len);
		*(uart_pc.cmd + len) = '\0';
		/* ִ������ */
		execute_cmd(uart_pc.cmd, len);
		/* �ͷ��ڴ� */
		free(uart_pc.cmd);
		/* ��ϴ���ջ��� */
//		memset(uart_pc.recv_buf, 0, uart_pc.recv_len);
		uart_pc.recv_len = 0;

		uart_pc.state = NORMAL;
	}
}
void calc_ad_value()
{
	u8 i;
	u8 j;

	if(adc.enable == DISABLE){
		return;
	}
	/* ad_value[N][M]
	 * {ch0, ch1, ch2, ch3, ...}, N0
	 * {ch0, ch1, ch2, ch3, ...}, N1
	 * {ch0, ch1, ch2, ch3, ...}, N2
	 * ...
	 * {M0, M1, M2, M3}
	 */
	/* ����ƽ��ֵ��M��ͨ��, ÿ��ͨ��N���ɼ�ֵ��*/
	for(i = 0;i < M;i++){
		adc.sum = 0;
		for(j = 0;j < N;j++){
			adc.sum += ad_value[j][i];
		}
		adc.sum = adc.sum / N;
		ad_filter[i] = ((float)adc.sum) / 4096.0 * 3.3;
	}
	/* ת����ʵ�ʵ�ֵ */
	adc_val = (struct _adc_val*)ad_filter;
	adc_val->v24 *= 7.8;
	adc_val->v6 *= 2.0;
	adc_val->v5 *= 2.0;
	adc_val->v3d3 *= 1.1;
	adc_val->v12 *= 4.09;
	adc_val->v6m *= 2.0;
	adc_val->cur /= 2.0;
	adc_val->cur *= 1000; // mA
//	adc_val->tmp = (13.582 - sqrt(13.582*13.582 + 4*0.00433*(2230.8 - adc_val->tmp*1000))) / (2*-0.00433) + 30;

	adc.state = 0;
	if(adc_val->v24 > 25.5
	|| adc_val->v24 < 21.5){// 21.5~25.5
		adc.state = 1;
	}
	if(adc_val->v12 > 12.5
	|| adc_val->v12 < 11.5){// 11.5~12.5
		adc.state = 1;
	}
	if(adc_val->v6 > 6.3
	|| adc_val->v6 < 5.7){// 5.7~6.3
		adc.state = 1;
	}
	if(adc_val->v5 > 5.1
	|| adc_val->v5 < 4.9){// 4.9~5.1
		adc.state = 1;
	}
	if(adc_val->v3d3 > 3.4
	|| adc_val->v3d3 < 3.2){// 3.2~3.4
		adc.state = 1;
	}
	/* LC���ϵ磬δ�����裬����Ӧ�ú�С */
	if(cmd_to_dut.enable == DISABLE){
		if(adc_val->cur > 100){
			adc.state = 1;
		}
	}else{
		if(adc_val->cur > 300){
			adc.state = 1;
		}
	}
	if(adc.state){
		adc.enable = DISABLE;
		DUT_PWR_OFF;// DUT power off
	}
}
void handle_flag()
{
	if(power_on_delay.enable == ENABLE){
		/* ��ʱ������ */
		power_on_delay.tmp++;
		if(power_on_delay.tmp >= 5e3){
			power_on_delay.tmp = 0;
			power_on_delay.enable = DISABLE;
			cmd_to_dut.enable = ENABLE;
		}
	}
	if(reply_to_pc.enable == ENABLE){
		reply_to_pc.enable = DISABLE;
		packet_hex(cmd_to_dut.state);
		uart_to_pc(uart_pc.send_buf, uart_pc.send_len);
	}
}
void part_of_power_on()
{
	/* ����Ƿ����ִ�д˺��� */
	if(cmd_to_dut.enable == DISABLE){
		return;
	}
	cmd_to_dut.enable = DISABLE;
	cmd_to_dut.state = GOOD;
	/********* open OLED *********/
	uart_to_dut(DUT_OLED_ON, sizeof(DUT_OLED_ON));
	delay_ms(150);
	/********* open LED *********/
	uart_to_dut(DUT_LED_ON, sizeof(DUT_LED_ON));
	delay_ms(10);
	/********* close beep *********/
	uart_to_dut(DUT_BEEP_OFF, sizeof(DUT_BEEP_OFF));

	reply_to_pc.enable = ENABLE;
}
/**
 * @brief �������ݵ�DUT��Ȼ�����DUT���������ݣ��г�ʱ����
 *
 * @param p Ҫ���͵�����
 * @param len ���ݳ���
 *
 * @retval ״̬�룬0�ɹ���-1ʧ��
 */
int uart_to_dut(const u8 *p, u8 len)
{
	uart_dut.recv_len = 0;
	uart_dut.timer.state = NORMAL;
	uart_dut.state = NORMAL;
	uart_dut_putln(p, len);
	timer2.state = NORMAL;
	timer2.clear();
	timer2.start();
	/* waiting for receiving data or time out */
	while((uart_dut.timer.state == NORMAL) && (timer2.state == NORMAL));
	if(uart_dut.timer.state != NORMAL){// �յ�����
		timer2.stop();
		return 0;
	}else{// δ�յ����ݣ���ʱ100mS
		/* �������仰��Ӧ��������ײ㺯����ִ�У�Ӧ��ͨ������ֵ�жϴ��� */
		adc.enable = DISABLE;
		DUT_PWR_OFF;// DUT power off
		cmd_to_dut.state = ERROR_COM;

		return -1;
	}
}
void test_v3d3(char* parameter)
{
	packet_float(adc_val->v3d3, 1, 2);
	uart_to_pc(uart_pc.send_buf, uart_pc.send_len);
}
void test_v5(char* parameter)
{
	packet_float(adc_val->v5, 1, 2);
	uart_to_pc(uart_pc.send_buf, uart_pc.send_len);
}
void test_v24(char* parameter)
{
	packet_float(adc_val->v24, 2, 2);
	uart_to_pc(uart_pc.send_buf, uart_pc.send_len);
}
void test_v6(char* parameter)
{
	packet_float(adc_val->v6, 1, 2);
	uart_to_pc(uart_pc.send_buf, uart_pc.send_len);
}
void test_v12(char* parameter)
{
	packet_float(adc_val->v12, 2, 2);
	uart_to_pc(uart_pc.send_buf, uart_pc.send_len);
}
void test_pwr_on(char* parameter)
{
	/* Ŀ����ϵ� */
	DUT_PWR_ON;
	delay_ms(200);
	/* ����ADC�ɼ� */
	adc.enable = ENABLE;
	/* ��λ��־λ������handle_flag()����  */
	power_on_delay.enable = ENABLE;
	power_on_delay.tmp = 0;

	/********* open beep *********/
//	uart_dut_putln(DUT_BEEP_ON, sizeof(DUT_BEEP_ON));
}
void test_pwr_off(char* parameter)
{
	adc.enable = DISABLE;
	memset(ad_filter, M, 0);
	memset((void*)&ad_value[0], M*N, 0);
	DUT_PWR_OFF;

	packet_hex(1);
	uart_to_pc(uart_pc.send_buf, uart_pc.send_len);
}
void test_current(char* parameter)
{
	packet_float(adc_val->cur, 4, 0);
	uart_to_pc(uart_pc.send_buf, uart_pc.send_len);
}
void test_barcode(char* parameter)
{
	u8 code[14];
	u32 crc;

	code[0] = 0x55;
	code[1] = 0xAA;// header
	code[2] = 0x00;// ID
	code[3] = 0x0A;// length = index(1) + command(5) + crc(4)
	code[4] = 0x08;// index
	// command
	code[5] = ((uart_pc.recv_buf[9] - '0') << 4) + (uart_pc.recv_buf[10] - '0');
	code[6] = ((uart_pc.recv_buf[11] - '0') << 4) + (uart_pc.recv_buf[12] - '0');
	code[7] = ((uart_pc.recv_buf[13] - '0') << 4) + (uart_pc.recv_buf[14] - '0');
	code[8] = ((uart_pc.recv_buf[15] - '0') << 4) + (uart_pc.recv_buf[16] - '0');
	code[9] = ((uart_pc.recv_buf[17] - '0') << 4) + (uart_pc.recv_buf[18] - '0');
	crc = u8CRC_CalcBlockCRC(code+2, sizeof(code)-6);
	code[10] = crc & 0xFF;
	code[11] = (crc>>8) & 0xFF;
	code[12] = (crc>>16) & 0xFF;
	code[13] = (crc>>24) & 0xFF;

	/* Ҫ��PC�˷������ݣ��������ʾ���ӱ�ռ�ã�Ӧ����PC�˵�bug */
	if(uart_to_dut(code, sizeof(code)) == 0){
		if(uart_dut.recv_buf[5] == 0){
			packet_hex(GOOD);
		}else{
			packet_hex(BAD);
		}
	}else{
		packet_hex(ERROR_COM);
	}
	uart_to_pc(uart_pc.send_buf, uart_pc.send_len);
	/* �˴��µ� */
	DUT_PWR_OFF;
}
void test_can(char* parameter)
{
	can.state = NORMAL;
	timer4.state = NORMAL;
	timer4.clear();
	timer4.start();
	if(uart_to_dut(DUT_CAN_CHECK, sizeof(DUT_CAN_CHECK)) == 0
	&& compare(uart_dut.recv_buf, DUT_CAN_ACK, uart_dut.recv_len) == 0){
		/* waiting for receiving data or time out */
		while((can.state == NORMAL) && timer4.state == NORMAL);
		if(can.state != NORMAL){// �յ�����
			timer4.stop();
			if(compare(can_rxbuf.Data, can_data, 8) == 0){
				packet_hex(GOOD);
			}else{
				packet_hex(BAD);
			}
		}else{// δ�յ����ݣ���ʱ100mS
			adc.enable = DISABLE;
			DUT_PWR_OFF;// DUT power off
			packet_hex(ERROR_COM);
		}
	}else{
		packet_hex(ERROR_COM);
	}
	uart_to_pc(uart_pc.send_buf, uart_pc.send_len);
}
void test_motor(char* parameter)
{
//	float cur1, cur2;
//	u16 tmp;

	delay_ms(10);
	/* δ�򿪶��ǰ����IOӦ��Ϊ�͵�ƽ */
	if(GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_4) == 0){
		exti_motor.state = NORMAL;
		GPIO_SetBits(GPIOB, GPIO_Pin_5);// �򿪶������
		delay_ms(20);
		calc_ad_value();
		/* ��ʱ�ĵ�ѹӦ�úܵ� */
		if(adc_val->v6m > 0.2){
			packet_hex(ERROR_VOL);
		}else if(uart_to_dut(DUT_MOTOR_ON, sizeof(DUT_MOTOR_ON)) == 0){
//			tmp = uart_dut.recv_buf[6] | uart_dut.recv_buf[7] << 8;
//			cur1 = tmp * 3.3 / 4096.0;// LC�ϴ��ĵ���ֵ
//			delay_ms(20);
//			/* ���������  */
//			calc_ad_value();
//			GPIO_ResetBits(GPIOB, GPIO_Pin_5);// �Ͽ��������
//			cur2 = adc_val->v6 - adc_val->v6m;// ��װ���Լ�����ĵ���ֵ
//			cur1 = cur2 / cur1;
//			/* ��������ڷ�Χ�ڣ�����IO�ĵ�ƽ�仯�� */
//			if(cur1 > 0.2 && cur1 < 1.8 && exti_motor.state != NORMAL){
//				packet_hex(GOOD);
//			}else{
//				packet_hex(BAD);
//			}
			if(uart_dut.recv_buf[5] == 0){
				packet_hex(GOOD);
			}else{
				packet_hex(BAD);
			}
		}else{
			packet_hex(ERROR_COM);
		}
	}else{
		packet_hex(ERROR_IO);
	}
	uart_to_dut(DUT_MOTOR_OFF, sizeof(DUT_MOTOR_OFF));
	uart_to_pc(uart_pc.send_buf, uart_pc.send_len);
}
void test_hall(char* parameter)
{
	if(uart_to_dut(DUT_HALL_CHECK, sizeof(DUT_HALL_CHECK)) == 0){
		packet_float(uart_dut.recv_buf[6], 1, 0);
		/* ��ʱӦ�÷���0���д��� */
		if(uart_dut.recv_buf[6] == 0){

			GPIO_SetBits(GPIOA, GPIO_Pin_12);
			delay_ms(100);
			/* ��ʱӦ�÷���1���޴��� */
			if(uart_to_dut(DUT_HALL_CHECK, sizeof(DUT_HALL_CHECK)) == 0){
				packet_float(uart_dut.recv_buf[6], 1, 0);
			}else{
				packet_hex(ERROR_COM);
			}
			GPIO_ResetBits(GPIOA, GPIO_Pin_12);
		}else{
			packet_hex(BAD);
		}
	}else{
		packet_hex(ERROR_COM);
	}
	uart_to_pc(uart_pc.send_buf, uart_pc.send_len);
}
void test_temp(char* parameter)
{
	if(uart_to_dut(DUT_TMP_CHECK, sizeof(DUT_TMP_CHECK)) == 0){
		packet_float(uart_dut.recv_buf[6], 1, 0);// �����¶�ֵ
	}else{
		packet_hex(ERROR_COM);
	}
	uart_to_pc(uart_pc.send_buf, uart_pc.send_len);
}
void force_pwr_on(char* parameter)
{
	/* Ŀ����ϵ� */
	DUT_PWR_ON;
	packet_hex(GOOD);
	uart_to_pc(uart_pc.send_buf, uart_pc.send_len);
}
void USART3_IRQHandler(void)
{
	if(USART_GetITStatus(USART3, USART_IT_RXNE) != RESET){
		/* ״̬���ԣ�������һ֡����δ�����꣬��ֹ���� */
		if(uart_dut.state != NORMAL){
			return;// TODO ����
		}
		/* �������� */
		uart_dut.recv_buf[uart_dut.recv_len] = USART3->DR & 0xFF;
		uart_dut.recv_len++;
		/* ��ֹ������� */
		if(uart_dut.recv_len >= MAX_LEN){
			uart_dut.recv_len = 0;// TODO ����
		}
		/* �����ʱ������ֵ�������ʱ���رգ���������ʱ�� */
		uart_dut.timer.cnt = 0;
		if(uart_dut.timer.enable == DISABLE){
			/* enable timer */
			uart_dut.timer.enable = ENABLE;
		}
		USART_ClearITPendingBit(USART3, USART_IT_RXNE);
	}
}
void USART1_IRQHandler(void)
{
	if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET){
		/* ״̬���ԣ�������һ֡����δ�����꣬��ֹ���� */
		if(uart_pc.state != NORMAL){
			return;// TODO ����
		}
		/* �������� */
		uart_pc.recv_buf[uart_pc.recv_len] = USART1->DR & 0xFF;
		uart_pc.recv_len++;
		/* ��ֹ������� */
		if(uart_pc.recv_len >= MAX_LEN){
			uart_pc.recv_len = 0;// TODO ����
		}
		/* �����ʱ������ֵ�������ʱ���رգ���������ʱ�� */
		uart_pc.timer.cnt = 0;
		if(uart_pc.timer.enable == DISABLE){
			/* enable timer */
			uart_pc.timer.enable = ENABLE;
		}
	}
}
void USB_LP_CAN1_RX0_IRQHandler(void)
{
	can.state = RECEIVED;
	CAN_ClearITPendingBit(CAN1, CAN_IT_FMP0);
	CAN_Receive(CAN1, CAN_FIFO0, &can_rxbuf);
}
void EXTI4_IRQHandler(void)
{
	exti_motor.state = RECEIVED;
	EXTI_ClearITPendingBit(EXTI_Line4);
}
void TIM2_IRQHandler(void)
{
	if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET) {
		TIM_ClearITPendingBit(TIM2, TIM_IT_Update);

		timer2.stop();
		timer2.enable = DISABLE;
		timer2.state = TIME_OUT;
	}
}
void TIM3_IRQHandler(void)
{
	if (TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET) {
		TIM3->SR = ~TIM_IT_Update;
		if(uart_pc.timer.enable == ENABLE){
			uart_pc.timer.cnt++;
			if(uart_pc.timer.cnt > 1){
				/* disable timer */
				uart_pc.timer.enable = DISABLE;
				/* ��λ��־λ������ verify_pc_data()���� */
				uart_pc.timer.state = TIME_OUT;
			}
		}
		if(uart_dut.timer.enable == ENABLE){
			uart_dut.timer.cnt++;
			if(uart_dut.timer.cnt > 1){
				/* disable timer */
				uart_dut.timer.enable = DISABLE;
				/* ��λ��־λ */
				uart_dut.timer.state = TIME_OUT;
			}
		}
	}
}
void TIM4_IRQHandler(void)
{
	if (TIM_GetITStatus(TIM4, TIM_IT_Update) != RESET) {
		TIM4->SR = ~TIM_IT_Update;

		timer4.stop();
		timer4.enable = DISABLE;
		timer4.state = TIME_OUT;
	}
}
