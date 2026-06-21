#include "i2c.h"
#include "e2.h"



/*********************************************************************************************************
������:     get_e2_address
��ڲ���:   e2��ʼ��ַ
���ڲ���:   �� 
����ֵ:     eborad_addressʵ�ʵõ��Ľṹ���ַ
����:       zzz
����:       2023/3/31
��������:   �õ�pca9685 i2c��ַ,����ʼ����ӦоƬ
**********************************************************************************************************/
i2c_addr_def get_e2_address(uint8_t address)
{
     i2c_addr_def eboard_addess;

	   eboard_addess.flag = 0;
	
	   if(i2c_addr_poll(I2C0,address))
		 {
			    eboard_addess.periph = I2C0;
				  eboard_addess.addr = address;
					eboard_addess.flag = 1;			
		 } 			 
	   if(eboard_addess.flag != 1)
		 {
			    if(i2c_addr_poll(I2C1,address))
					{
						  eboard_addess.periph = I2C1;
				      eboard_addess.addr = address;
					    eboard_addess.flag = 1; 
          }						
     }	
     
     if(eboard_addess.flag)
		 pca9685_e2_init(eboard_addess.periph,eboard_addess.addr);	
			 		 
	 return eboard_addess;		 
} 


/*********************************************************************************************************
������:     set_e2_pca9685_pwm_off
��ڲ���:   i2c_periph I2C��ѡ�� i2c0 or i2c1  i2c_addr ������ַ  
���ڲ���:   �� 
����:       zzz
����:       2023/3/28
��������:   �ر�pca9685����pwm�����
**********************************************************************************************************/
void set_e2_pca9685_pwm_off(uint32_t i2c_periph,uint8_t i2c_addr) 
{
    i2c_byte_write(i2c_periph,i2c_addr,ALLCN_ON_L,0);
    i2c_byte_write(i2c_periph,i2c_addr,ALLCN_ON_H,0);
    i2c_byte_write(i2c_periph,i2c_addr,ALLCN_OFF_L,0);
    i2c_byte_write(i2c_periph,i2c_addr,ALLCN_OFF_H,0x10);
}


/*********************************************************************************************************
������:     pc9685_e2_init
��ڲ���:   i2c_periph I2C��ѡ�� i2c0 or i2c1  i2c_addr ������ַ  
���ڲ���:   �� 
����:       zzz
����:       2023/3/28
��������:   ��ʼ��pca9685����оƬ
***********************************************************************************************************/
void pca9685_e2_init(uint32_t i2c_periph,uint8_t i2c_addr)
{
	i2c_delay_byte_write(i2c_periph,i2c_addr,E2_PCA9685_MODE1,0x0); //ģʽ��ʼ��
	set_e2_pca9685_pwm_off(i2c_periph,i2c_addr);   
}


/*********************************************************************************************************
������:     set_e2_pca9685_pwm
��ڲ���:   i2c_periph I2C��ѡ�� i2c0 or i2c1  i2c_addr ������ַ  
            num PCA9685��Ӧ������� on���ʹ�ܼ���ֵ off����رռ���ֵ��0-15��
���ڲ���:   �� 
����:       zzz
����:       2023/3/28
��������:   pwm duty = (off - on)/4095   ͨ��on��off��ֵ����ռ�ձ�
***********************************************************************************************************/
void set_e2_pca9685_pwm(uint32_t i2c_periph,uint8_t i2c_addr,uint8_t num, uint16_t on, uint16_t off) 
{

    i2c_delay_byte_write(i2c_periph,i2c_addr,CN0_ON_L+4*num,on);
    i2c_delay_byte_write(i2c_periph,i2c_addr,CN0_ON_H+4*num,on>>8);
    i2c_delay_byte_write(i2c_periph,i2c_addr,CN0_OFF_L+4*num,off);
    i2c_delay_byte_write(i2c_periph,i2c_addr,CN0_OFF_H+4*num,off>>8);
}


/*********************************************************************************************************
������:     e2_init
��ڲ���:   e2��ַ 
���ڲ���:   �� 
����ֵ:     i2c_addr_def����ṹ��
����:       zzz
����:       2023/3/29
��������:   �õ�e2 pca9685 i2c��ַ,��������������ṹ��flagֵΪ0,���������ʼ��оƬ�ýṹ��flagֵΪ1
**********************************************************************************************************/
i2c_addr_def e2_init(uint8_t address)
{
	i2c_addr_def e_addess;
	uint8_t i;

	e_addess.flag = 0;

	for(i=0;i<4;i++)
	{
		if(i2c_addr_poll(I2C0,address+i*2))
		{
			e_addess.periph = I2C0;
			e_addess.addr = address+i*2;
			e_addess.flag = 1;			
			break;
		} 
	}
	if(e_addess.flag != 1)
	{			
		for(i=0;i<4;i++)
		{
			if(i2c_addr_poll(I2C1,address+i*2))
			{
				e_addess.periph = I2C1;
				e_addess.addr = address+i*2;
				e_addess.flag = 1;
				break;
			}	
		}
	}

	if(e_addess.flag)
		pca9685_e2_init(e_addess.periph,e_addess.addr);	

	return e_addess;		 
}


/*********************************************************************************************************
������:     e2_all_init
��ڲ���:   e2_address�ṹ��ָ��  e2_addr-pca9685��ʼ��ַ 
���ڲ���:   e2_address�ṹ��ָ��
����ֵ:     ��
����:       zzz
����:       2023/3/31
��������:   ���Ҳ���ʼ������E2�Ӱ�,���õ���Ӧ�ĵ�ַ���ڽṹ��ָ��e2_address
**********************************************************************************************************/
void e2_all_init(out e2_addr_def *e2_address,uint8_t e2_addr)
{
	uint8_t i;

	for(i=0;i<4;i++)
	{
		e2_address->motor_addr[i] = get_e2_address(e2_addr+i*2);	
	}
}



/*********************************************************************************************
������:     e2_speed_control
��ڲ���:   e2 i2c�� e2��ַ, speed_levelת�ٰٷֱ�(0-100)
���ڲ���:   �� 
����ֵ:     ��
����:       zzz
����:       2023/3/29
��������:   ���Ʒ���ת��
**********************************************************************************************/
void e2_speed_control(uint32_t i2c_periph,uint8_t i2c_addr,uint8_t speed_level)
{
	uint16_t off,on;

	if(speed_level > 100)
		speed_level = 100;

	on = 0x00;
	off = on + 0xfff*speed_level/100;
	set_e2_pca9685_pwm(i2c_periph,i2c_addr,0,on,off);
}