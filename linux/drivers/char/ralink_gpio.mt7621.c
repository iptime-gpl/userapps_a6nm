/*
 ***************************************************************************
 * Ralink Tech Inc.
 * 4F, No. 2 Technology 5th Rd.
 * Science-based Industrial Park
 * Hsin-chu, Taiwan, R.O.C.
 *
 * (c) Copyright, Ralink Technology, Inc.
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 *  THIS  SOFTWARE  IS PROVIDED   ``AS  IS'' AND   ANY  EXPRESS OR IMPLIED
 *  WARRANTIES,   INCLUDING, BUT NOT  LIMITED  TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN
 *  NO  EVENT  SHALL   THE AUTHOR  BE    LIABLE FOR ANY   DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 *  NOT LIMITED   TO, PROCUREMENT OF  SUBSTITUTE GOODS  OR SERVICES; LOSS OF
 *  USE, DATA,  OR PROFITS; OR  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 *  ANY THEORY OF LIABILITY, WHETHER IN  CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 *  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *  You should have received a copy of the  GNU General Public License along
 *  with this program; if not, write  to the Free Software Foundation, Inc.,
 *  675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *
 ***************************************************************************
 *
 */
#include <linux/init.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <linux/sched.h>
#ifdef CONFIG_RALINK_GPIO_LED
#include <linux/timer.h>
#endif
#include <asm/uaccess.h>
#include "ralink_gpio.h"

#include <asm/rt2880/surfboardint.h>

#ifdef  CONFIG_DEVFS_FS
#include <linux/devfs_fs_kernel.h>
static  devfs_handle_t devfs_handle;
#endif

#define NAME			"ralink_gpio"
#define RALINK_GPIO_DEVNAME	"gpio"
int ralink_gpio_major = 252;
int ralink_gpio_irqnum = 0;
u32 ralink_gpio_intp = 0;
u32 ralink_gpio_edge = 0;
#if defined (RALINK_GPIO_HAS_2722)
u32 ralink_gpio2722_intp = 0;
u32 ralink_gpio2722_edge = 0;
#elif defined (RALINK_GPIO_HAS_4524)
u32 ralink_gpio3924_intp = 0;
u32 ralink_gpio3924_edge = 0;
u32 ralink_gpio4540_intp = 0;
u32 ralink_gpio4540_edge = 0;
#elif defined (RALINK_GPIO_HAS_5124)
u32 ralink_gpio3924_intp = 0;
u32 ralink_gpio3924_edge = 0;
u32 ralink_gpio5140_intp = 0;
u32 ralink_gpio5140_edge = 0;
#elif defined (RALINK_GPIO_HAS_9524) || defined (RALINK_GPIO_HAS_7224)
u32 ralink_gpio3924_intp = 0;
u32 ralink_gpio3924_edge = 0;
u32 ralink_gpio7140_intp = 0;
u32 ralink_gpio7140_edge = 0;
#if defined (RALINK_GPIO_HAS_7224)
u32 ralink_gpio72_intp = 0;
u32 ralink_gpio72_edge = 0;
#else
u32 ralink_gpio9572_intp = 0;
u32 ralink_gpio9572_edge = 0;
#endif
#elif defined (RALINK_GPIO_HAS_9532)
u32 ralink_gpio6332_intp = 0;
u32 ralink_gpio6332_edge = 0;
u32 ralink_gpio9564_intp = 0;
u32 ralink_gpio9564_edge = 0;
#endif
ralink_gpio_reg_info ralink_gpio_info[RALINK_GPIO_NUMBER];
extern unsigned long volatile jiffies;

#ifdef CONFIG_RALINK_GPIO_LED
#define RALINK_LED_DEBUG 0
#define RALINK_GPIO_LED_FREQ (HZ/10)
struct timer_list ralink_gpio_led_timer;
ralink_gpio_led_info ralink_gpio_led_data[RALINK_GPIO_NUMBER];

u32 ra_gpio_led_set = 0;
u32 ra_gpio_led_clr = 0;
#if defined (RALINK_GPIO_HAS_2722)
u32 ra_gpio2722_led_set = 0;
u32 ra_gpio2722_led_clr = 0;
#elif defined (RALINK_GPIO_HAS_4524)
u32 ra_gpio3924_led_set = 0;
u32 ra_gpio3924_led_clr = 0;
u32 ra_gpio4540_led_set = 0;
u32 ra_gpio4540_led_clr = 0;
#elif defined (RALINK_GPIO_HAS_5124)
u32 ra_gpio3924_led_set = 0;
u32 ra_gpio3924_led_clr = 0;
u32 ra_gpio5140_led_set = 0;
u32 ra_gpio5140_led_clr = 0;
#elif defined (RALINK_GPIO_HAS_9524) || defined (RALINK_GPIO_HAS_7224)
u32 ra_gpio3924_led_set = 0;
u32 ra_gpio3924_led_clr = 0;
u32 ra_gpio7140_led_set = 0;
u32 ra_gpio7140_led_clr = 0;
#if defined (RALINK_GPIO_HAS_7224)
u32 ra_gpio72_led_set = 0;
u32 ra_gpio72_led_clr = 0;
#else
u32 ra_gpio9572_led_set = 0;
u32 ra_gpio9572_led_clr = 0;
#endif
#elif defined (RALINK_GPIO_HAS_9532)
u32 ra_gpio6332_led_set = 0;
u32 ra_gpio6332_led_clr = 0;
u32 ra_gpio9564_led_set = 0;
u32 ra_gpio9564_led_clr = 0;
#endif
struct ralink_gpio_led_status_t {
	int ticks;
	unsigned int ons;
	unsigned int offs;
	unsigned int resting;
	unsigned int times;
} ralink_gpio_led_stat[RALINK_GPIO_NUMBER];
#endif
void ralink_gpio_notify_user(int usr);
static struct work_struct gpio_event_hold;
static struct work_struct gpio_event_click;


MODULE_DESCRIPTION("Ralink SoC GPIO Driver");
MODULE_AUTHOR("Winfred Lu <winfred_lu@ralinktech.com.tw>");
MODULE_LICENSE("GPL");
ralink_gpio_reg_info info;


#ifdef CONFIG_EFM_PATCH
 
#define atoi(str) simple_strtoul(((str != NULL) ? str : ""), NULL, 0)

int gpio_profile;
#define EFM_GPIO_PROFILE_1 1
#define EFM_GPIO_PROFILE_2 2
#define EFM_GPIO_PROFILE_3 3
#define EFM_GPIO_PROFILE_4 4

#define EFM_GPIO_PROFILE_COMMON 100

int reset_gpio_num = -1;
int wps_gpio_num = -1;
int runled_gpio_num = -1;
int usbled_gpio_num = -1;
int apstatus_led_gpio_num = -1;

#define SIGNALING_PID 1
#define RESET_SIGNAL

int use_wps_only_button;

int ralink_gpio_led_set(ralink_gpio_led_info led);

void blink_led(int gpio_no, int period)
{
        if(gpio_no == -1) return;

        /* Run LED init */
        ralink_gpio_led_data[gpio_no].gpio = gpio_no;
        ralink_gpio_led_data[gpio_no].on = period;
        ralink_gpio_led_data[gpio_no].off = period;
        ralink_gpio_led_data[gpio_no].blinks = RALINK_GPIO_LED_INFINITY;
        ralink_gpio_led_data[gpio_no].rests = 0;
        ralink_gpio_led_data[gpio_no].times = RALINK_GPIO_LED_INFINITY;

        ralink_gpio_led_stat[gpio_no].ticks = -1;
        ralink_gpio_led_stat[gpio_no].ons = 0;
        ralink_gpio_led_stat[gpio_no].offs = 0;
        ralink_gpio_led_stat[gpio_no].resting = 0;
        ralink_gpio_led_stat[gpio_no].times = 0;

        ralink_gpio_led_set(ralink_gpio_led_data[gpio_no]);

        return;
}


void led_onoff(int gpio_no, int onoff)
{
        if(gpio_no == -1) return;

        /* Run LED init */
        ralink_gpio_led_data[gpio_no].gpio = gpio_no;
        ralink_gpio_led_data[gpio_no].on = onoff?RALINK_GPIO_LED_INFINITY:0;
        ralink_gpio_led_data[gpio_no].off = onoff?0:RALINK_GPIO_LED_INFINITY;
        ralink_gpio_led_data[gpio_no].blinks = 0;
        ralink_gpio_led_data[gpio_no].rests = 0;
        ralink_gpio_led_data[gpio_no].times = RALINK_GPIO_LED_INFINITY;

        ralink_gpio_led_stat[gpio_no].ticks = -1;
        ralink_gpio_led_stat[gpio_no].ons = 0;
        ralink_gpio_led_stat[gpio_no].offs = 0;
        ralink_gpio_led_stat[gpio_no].resting = 0;
        ralink_gpio_led_stat[gpio_no].times = 0;

        ralink_gpio_led_set(ralink_gpio_led_data[gpio_no]);

        return;
}


void init_iptime_gpio_standard(void)
{
        unsigned long tmp;

        if(!gpio_profile)
        {
                printk("GPIO Profile is not set\n");
                return;
        }

        if(runled_gpio_num != -1) blink_led( runled_gpio_num, 10 );
        if(usbled_gpio_num != -1) led_onoff( usbled_gpio_num, 0);
#ifdef  CONFIG_RALINK_MT7620
        led_onoff( 44, 1); /* For Port #4 */
#endif

#if defined CONFIG_RALINK_MT7620 || defined CONFIG_RALINK_MT7621
        if(reset_gpio_num != -1)
        {
                ralink_gpio_info[reset_gpio_num].pid = SIGNALING_PID;

                tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIODIR));
                tmp &= ~(0x1 << reset_gpio_num);
                *(volatile u32 *)(RALINK_REG_PIODIR) = cpu_to_le32(tmp);

                tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIORENA));
                tmp |= (0x1 << reset_gpio_num);
                *(volatile u32 *)(RALINK_REG_PIORENA) = cpu_to_le32(tmp);

                tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIOFENA));
                tmp |= (0x1 << reset_gpio_num);
                *(volatile u32 *)(RALINK_REG_PIOFENA) = cpu_to_le32(tmp);
        }

        if((wps_gpio_num != -1) && (wps_gpio_num != reset_gpio_num))
        {
                ralink_gpio_info[wps_gpio_num].pid = SIGNALING_PID;

                tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIODIR));
                tmp &= ~(0x1 << wps_gpio_num);
                *(volatile u32 *)(RALINK_REG_PIODIR) = cpu_to_le32(tmp);


                tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIORENA));
                tmp |= (0x1 << wps_gpio_num);
                *(volatile u32 *)(RALINK_REG_PIORENA) = cpu_to_le32(tmp);

                tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIOFENA));
                tmp |= (0x1 << wps_gpio_num);
                *(volatile u32 *)(RALINK_REG_PIOFENA) = cpu_to_le32(tmp);
        }
#endif


}

int check_time;
int reset_timer;
int reset_pushed;


static int check_reset_bt(void)
{
        u32 value;

        value = *(volatile u32 *)(RALINK_REG_PIODATA);
        if(value & (0x1<<(reset_gpio_num)))
                return 0; /* not pushed */
        else
                return 1; /* pushed */
}

int check_default_reset(void)
{
        if(!gpio_profile) return 0;

        if((jiffies  - check_time ) < HZ )
                return 0;
        check_time = jiffies;

        check_time = jiffies;

        if(!check_reset_bt())
        {
                reset_timer = 0;
                if(reset_pushed)
                {
                        blink_led( runled_gpio_num, 10 );
                        reset_pushed = 0;
                }
        }
        else
        {
		int timeout;

                reset_timer++;
                printk("Reset Click %d\n", reset_timer);
        	if((wps_gpio_num == reset_gpio_num))
			timeout=8;
		else
			timeout=5;

                if(reset_timer >= timeout)
                {
                        printk("Reset\n");
			if(apstatus_led_gpio_num != -1)
				led_onoff(apstatus_led_gpio_num,0);
                        blink_led( runled_gpio_num, 1 );
                        /* real signal will be generated by irq */
                        reset_pushed = 1;
                }
        }
        return 0;
}
#endif



void gpio_click_notify(struct work_struct *work)
{
    //printk("<hua-dbg> %s, 1\n", __FUNCTION__);
    ralink_gpio_notify_user(1);
}


void gpio_hold_notify(struct work_struct *work)
{
    //printk("<hua-dbg> %s, 2\n", __FUNCTION__);
    ralink_gpio_notify_user(2);
}


int ralink_gpio_led_set(ralink_gpio_led_info led)
{
#ifdef CONFIG_RALINK_GPIO_LED
	unsigned long tmp;
	if (0 <= led.gpio && led.gpio < RALINK_GPIO_NUMBER) {
		if (led.on > RALINK_GPIO_LED_INFINITY)
			led.on = RALINK_GPIO_LED_INFINITY;
		if (led.off > RALINK_GPIO_LED_INFINITY)
			led.off = RALINK_GPIO_LED_INFINITY;
		if (led.blinks > RALINK_GPIO_LED_INFINITY)
			led.blinks = RALINK_GPIO_LED_INFINITY;
		if (led.rests > RALINK_GPIO_LED_INFINITY)
			led.rests = RALINK_GPIO_LED_INFINITY;
		if (led.times > RALINK_GPIO_LED_INFINITY)
			led.times = RALINK_GPIO_LED_INFINITY;
		if (led.on == 0 && led.off == 0 && led.blinks == 0 &&
				led.rests == 0) {
			ralink_gpio_led_data[led.gpio].gpio = -1; //stop it
			return 0;
		}
		//register led data
		ralink_gpio_led_data[led.gpio].gpio = led.gpio;
		ralink_gpio_led_data[led.gpio].on = (led.on == 0)? 1 : led.on;
		ralink_gpio_led_data[led.gpio].off = (led.off == 0)? 1 : led.off;
		ralink_gpio_led_data[led.gpio].blinks = (led.blinks == 0)? 1 : led.blinks;
		ralink_gpio_led_data[led.gpio].rests = (led.rests == 0)? 1 : led.rests;
		ralink_gpio_led_data[led.gpio].times = (led.times == 0)? 1 : led.times;

		//clear previous led status
		ralink_gpio_led_stat[led.gpio].ticks = -1;
		ralink_gpio_led_stat[led.gpio].ons = 0;
		ralink_gpio_led_stat[led.gpio].offs = 0;
		ralink_gpio_led_stat[led.gpio].resting = 0;
		ralink_gpio_led_stat[led.gpio].times = 0;

		printk("led=%d, on=%d, off=%d, blinks,=%d, reset=%d, time=%d\n",
				ralink_gpio_led_data[led.gpio].gpio,
				ralink_gpio_led_data[led.gpio].on,
				ralink_gpio_led_data[led.gpio].off,
				ralink_gpio_led_data[led.gpio].blinks,
				ralink_gpio_led_data[led.gpio].rests,
				ralink_gpio_led_data[led.gpio].times);
		//set gpio direction to 'out'
#if defined (RALINK_GPIO_HAS_2722)
		if (led.gpio <= 21) {
			tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIODIR));
			tmp |= RALINK_GPIO(led.gpio);
			*(volatile u32 *)(RALINK_REG_PIODIR) = tmp;
		}
		else {
			tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO2722DIR));
			tmp |= RALINK_GPIO((led.gpio-22));
			*(volatile u32 *)(RALINK_REG_PIO2722DIR) = tmp;
		}
#elif defined (RALINK_GPIO_HAS_9532)
		if (led.gpio <= 31) {
			tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIODIR));
			tmp |= RALINK_GPIO(led.gpio);
			*(volatile u32 *)(RALINK_REG_PIODIR) = tmp;
		} else if (led.gpio <= 63) {
			tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO6332DIR));
			tmp |= RALINK_GPIO((led.gpio-32));
			*(volatile u32 *)(RALINK_REG_PIO6332DIR) = tmp;
		} else {
			tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO9564DIR));
			tmp |= RALINK_GPIO((led.gpio-64));
			*(volatile u32 *)(RALINK_REG_PIO9564DIR) = tmp;
		}
#else
		if (led.gpio <= 23) {
			tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIODIR));
			tmp |= RALINK_GPIO(led.gpio);
			*(volatile u32 *)(RALINK_REG_PIODIR) = tmp;
		}
#if defined (RALINK_GPIO_HAS_4524)
		else if (led.gpio <= 39) {
			tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO3924DIR));
			tmp |= RALINK_GPIO((led.gpio-24));
			*(volatile u32 *)(RALINK_REG_PIO3924DIR) = tmp;
		}
		else {
			tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO4540DIR));
			tmp |= RALINK_GPIO((led.gpio-40));
			*(volatile u32 *)(RALINK_REG_PIO4540DIR) = tmp;
		}
#elif defined (RALINK_GPIO_HAS_5124)
		else if (led.gpio <= 39) {
			tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO3924DIR));
			tmp |= RALINK_GPIO((led.gpio-24));
			*(volatile u32 *)(RALINK_REG_PIO3924DIR) = tmp;
		}
		else {
			tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO5140DIR));
			tmp |= RALINK_GPIO((led.gpio-40));
			*(volatile u32 *)(RALINK_REG_PIO5140DIR) = tmp;
		}
#elif defined (RALINK_GPIO_HAS_9524) || defined (RALINK_GPIO_HAS_7224)
		else if (led.gpio <= 39) {
			tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO3924DIR));
			tmp |= RALINK_GPIO((led.gpio-24));
			*(volatile u32 *)(RALINK_REG_PIO3924DIR) = tmp;
		}
		else if (led.gpio <= 71) {
			tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO7140DIR));
			tmp |= RALINK_GPIO((led.gpio-40));
			*(volatile u32 *)(RALINK_REG_PIO7140DIR) = tmp;
		}
		else {
#if defined (RALINK_GPIO_HAS_7224)
			tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO72DIR));
			tmp |= RALINK_GPIO((led.gpio-72));
			*(volatile u32 *)(RALINK_REG_PIO72DIR) = tmp;
#else
			tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO9572DIR));
			tmp |= RALINK_GPIO((led.gpio-72));
			*(volatile u32 *)(RALINK_REG_PIO9572DIR) = tmp;
#endif
		}
#endif
#endif
#if RALINK_LED_DEBUG
		printk("dir_%x gpio_%d - %d %d %d %d %d\n", tmp,
				led.gpio, led.on, led.off, led.blinks,
				led.rests, led.times);
#endif
	}
	else {
		printk(KERN_ERR NAME ": gpio(%d) out of range\n", led.gpio);
		return -1;
	}
	return 0;
#else
	printk(KERN_ERR NAME ": gpio led support not built\n");
	return -1;
#endif
}
EXPORT_SYMBOL(ralink_gpio_led_set);

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,35)
long ralink_gpio_ioctl(struct file *file, unsigned int req,
		unsigned long arg)
#else
int ralink_gpio_ioctl(struct inode *inode, struct file *file, unsigned int req,
		unsigned long arg)
#endif
{
	unsigned long tmp;
	ralink_gpio_reg_info info;
#ifdef CONFIG_RALINK_GPIO_LED
	ralink_gpio_led_info led;
#endif

	req &= RALINK_GPIO_DATA_MASK;

	switch(req) {
	case RALINK_GPIO_SET_DIR:
		*(volatile u32 *)(RALINK_REG_PIODIR) = cpu_to_le32(arg);
		break;
	case RALINK_GPIO_SET_DIR_IN:
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIODIR));
		tmp &= ~cpu_to_le32(arg);
		*(volatile u32 *)(RALINK_REG_PIODIR) = tmp;
		break;
	case RALINK_GPIO_SET_DIR_OUT:
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIODIR));
		tmp |= cpu_to_le32(arg);
		*(volatile u32 *)(RALINK_REG_PIODIR) = tmp;
		break;
	case RALINK_GPIO_READ: //RALINK_GPIO_READ_INT
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIODATA));
		put_user(tmp, (int __user *)arg);
		break;
	case RALINK_GPIO_WRITE: //RALINK_GPIO_WRITE_INT
		*(volatile u32 *)(RALINK_REG_PIODATA) = cpu_to_le32(arg);
		break;
	case RALINK_GPIO_SET: //RALINK_GPIO_SET_INT
		*(volatile u32 *)(RALINK_REG_PIOSET) = cpu_to_le32(arg);
		break;
	case RALINK_GPIO_CLEAR: //RALINK_GPIO_CLEAR_INT
		*(volatile u32 *)(RALINK_REG_PIORESET) = cpu_to_le32(arg);
		break;
	case RALINK_GPIO_ENABLE_INTP:
		*(volatile u32 *)(RALINK_REG_INTENA) = cpu_to_le32(RALINK_INTCTL_PIO);
		break;
	case RALINK_GPIO_DISABLE_INTP:
		*(volatile u32 *)(RALINK_REG_INTDIS) = cpu_to_le32(RALINK_INTCTL_PIO);
		break;
	case RALINK_GPIO_REG_IRQ:
		copy_from_user(&info, (ralink_gpio_reg_info *)arg, sizeof(info));
		if (0 <= info.irq && info.irq < RALINK_GPIO_NUMBER) {
			ralink_gpio_info[info.irq].pid = info.pid;
#if defined (RALINK_GPIO_HAS_2722)
			if (info.irq <= 21) {
				tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIORENA));
				tmp |= (0x1 << info.irq);
				*(volatile u32 *)(RALINK_REG_PIORENA) = cpu_to_le32(tmp);
				tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIOFENA));
				tmp |= (0x1 << info.irq);
				*(volatile u32 *)(RALINK_REG_PIOFENA) = cpu_to_le32(tmp);
			}
			else {
				tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO2722RENA));
				tmp |= (0x1 << (info.irq-22));
				*(volatile u32 *)(RALINK_REG_PIO2722RENA) = cpu_to_le32(tmp);
				tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO2722FENA));
				tmp |= (0x1 << (info.irq-22));
				*(volatile u32 *)(RALINK_REG_PIO2722FENA) = cpu_to_le32(tmp);
			}
#elif defined (RALINK_GPIO_HAS_9532)
			if (info.irq <= 31) {
				tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIORENA));
				tmp |= (0x1 << info.irq);
				*(volatile u32 *)(RALINK_REG_PIORENA) = cpu_to_le32(tmp);
				tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIOFENA));
				tmp |= (0x1 << info.irq);
				*(volatile u32 *)(RALINK_REG_PIOFENA) = cpu_to_le32(tmp);
			} else if (info.irq <= 63) {
				tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO6332RENA));
				tmp |= (0x1 << (info.irq-32));
				*(volatile u32 *)(RALINK_REG_PIO6332RENA) = cpu_to_le32(tmp);
				tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO6332FENA));
				tmp |= (0x1 << (info.irq-32));
				*(volatile u32 *)(RALINK_REG_PIO6332FENA) = cpu_to_le32(tmp);
			} else {
				tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO9564RENA));
				tmp |= (0x1 << (info.irq-64));
				*(volatile u32 *)(RALINK_REG_PIO9564RENA) = cpu_to_le32(tmp);
				tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO9564FENA));
				tmp |= (0x1 << (info.irq-64));
				*(volatile u32 *)(RALINK_REG_PIO9564FENA) = cpu_to_le32(tmp);
			}
#else
			if (info.irq <= 23) {
				tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIORENA));
				tmp |= (0x1 << info.irq);
				*(volatile u32 *)(RALINK_REG_PIORENA) = cpu_to_le32(tmp);
				tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIOFENA));
				tmp |= (0x1 << info.irq);
				*(volatile u32 *)(RALINK_REG_PIOFENA) = cpu_to_le32(tmp);
			}
#if defined (RALINK_GPIO_HAS_4524)
			else if (info.irq <= 39) {
				tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO3924RENA));
				tmp |= (0x1 << (info.irq-24));
				*(volatile u32 *)(RALINK_REG_PIO3924RENA) = cpu_to_le32(tmp);
				tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO3924FENA));
				tmp |= (0x1 << (info.irq-24));
				*(volatile u32 *)(RALINK_REG_PIO3924FENA) = cpu_to_le32(tmp);
			}
			else {
				tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO4540RENA));
				tmp |= (0x1 << (info.irq-40));
				*(volatile u32 *)(RALINK_REG_PIO4540RENA) = cpu_to_le32(tmp);
				tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO4540FENA));
				tmp |= (0x1 << (info.irq-40));
				*(volatile u32 *)(RALINK_REG_PIO4540FENA) = cpu_to_le32(tmp);
			}
#elif defined (RALINK_GPIO_HAS_5124)
			else if (info.irq <= 39) {
				tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO3924RENA));
				tmp |= (0x1 << (info.irq-24));
				*(volatile u32 *)(RALINK_REG_PIO3924RENA) = cpu_to_le32(tmp);
				tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO3924FENA));
				tmp |= (0x1 << (info.irq-24));
				*(volatile u32 *)(RALINK_REG_PIO3924FENA) = cpu_to_le32(tmp);
			}
			else {
				tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO5140RENA));
				tmp |= (0x1 << (info.irq-40));
				*(volatile u32 *)(RALINK_REG_PIO5140RENA) = cpu_to_le32(tmp);
				tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO5140FENA));
				tmp |= (0x1 << (info.irq-40));
				*(volatile u32 *)(RALINK_REG_PIO5140FENA) = cpu_to_le32(tmp);
			}
#elif defined (RALINK_GPIO_HAS_9524) || defined (RALINK_GPIO_HAS_7224)
			else if (info.irq <= 39) {
				tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO3924RENA));
				tmp |= (0x1 << (info.irq-24));
				*(volatile u32 *)(RALINK_REG_PIO3924RENA) = cpu_to_le32(tmp);
				tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO3924FENA));
				tmp |= (0x1 << (info.irq-24));
				*(volatile u32 *)(RALINK_REG_PIO3924FENA) = cpu_to_le32(tmp);
			}
			else if (info.irq <= 71) {
				tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO7140RENA));
				tmp |= (0x1 << (info.irq-40));
				*(volatile u32 *)(RALINK_REG_PIO7140RENA) = cpu_to_le32(tmp);
				tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO7140FENA));
				tmp |= (0x1 << (info.irq-40));
				*(volatile u32 *)(RALINK_REG_PIO7140FENA) = cpu_to_le32(tmp);
			}
			else {
#if defined (RALINK_GPIO_HAS_7224)
				tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO72RENA));
				tmp |= (0x1 << (info.irq-72));
				*(volatile u32 *)(RALINK_REG_PIO72RENA) = cpu_to_le32(tmp);
				tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO72FENA));
				tmp |= (0x1 << (info.irq-72));
				*(volatile u32 *)(RALINK_REG_PIO72FENA) = cpu_to_le32(tmp);
#else
				tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO9572RENA));
				tmp |= (0x1 << (info.irq-72));
				*(volatile u32 *)(RALINK_REG_PIO9572RENA) = cpu_to_le32(tmp);
				tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO9572FENA));
				tmp |= (0x1 << (info.irq-72));
				*(volatile u32 *)(RALINK_REG_PIO9572FENA) = cpu_to_le32(tmp);
#endif
			}
#endif
#endif
		}
		else
			printk(KERN_ERR NAME ": irq number(%d) out of range\n",
					info.irq);
		break;

#if defined (RALINK_GPIO_HAS_2722)
	case RALINK_GPIO2722_SET_DIR:
		*(volatile u32 *)(RALINK_REG_PIO2722DIR) = cpu_to_le32(arg);
		break;
	case RALINK_GPIO2722_SET_DIR_IN:
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO2722DIR));
		tmp &= ~cpu_to_le32(arg);
		*(volatile u32 *)(RALINK_REG_PIO2722DIR) = tmp;
		break;
	case RALINK_GPIO2722_SET_DIR_OUT:
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO2722DIR));
		tmp |= cpu_to_le32(arg);
		*(volatile u32 *)(RALINK_REG_PIO2722DIR) = tmp;
		break;
	case RALINK_GPIO2722_READ:
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO2722DATA));
		put_user(tmp, (int __user *)arg);
		break;
	case RALINK_GPIO2722_WRITE:
		*(volatile u32 *)(RALINK_REG_PIO2722DATA) = cpu_to_le32(arg);
		break;
	case RALINK_GPIO2722_SET:
		*(volatile u32 *)(RALINK_REG_PIO2722SET) = cpu_to_le32(arg);
		break;
	case RALINK_GPIO2722_CLEAR:
		*(volatile u32 *)(RALINK_REG_PIO2722SET) = cpu_to_le32(arg);
		break;
#elif defined (RALINK_GPIO_HAS_9532)
	case RALINK_GPIO6332_SET_DIR:
		*(volatile u32 *)(RALINK_REG_PIO6332DIR) = cpu_to_le32(arg);
		break;
	case RALINK_GPIO6332_SET_DIR_IN:
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO6332DIR));
		tmp &= ~cpu_to_le32(arg);
		*(volatile u32 *)(RALINK_REG_PIO6332DIR) = tmp;
		break;
	case RALINK_GPIO6332_SET_DIR_OUT:
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO6332DIR));
		tmp |= cpu_to_le32(arg);
		*(volatile u32 *)(RALINK_REG_PIO6332DIR) = tmp;
		break;
	case RALINK_GPIO6332_READ:
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO6332DATA));
		put_user(tmp, (int __user *)arg);
		break;
	case RALINK_GPIO6332_WRITE:
		*(volatile u32 *)(RALINK_REG_PIO6332DATA) = cpu_to_le32(arg);
		break;
	case RALINK_GPIO6332_SET:
		*(volatile u32 *)(RALINK_REG_PIO6332SET) = cpu_to_le32(arg);
		break;
	case RALINK_GPIO6332_CLEAR:
		*(volatile u32 *)(RALINK_REG_PIO6332SET) = cpu_to_le32(arg);
		break;
	case RALINK_GPIO9564_SET_DIR:
		*(volatile u32 *)(RALINK_REG_PIO9564DIR) = cpu_to_le32(arg);
		break;
	case RALINK_GPIO9564_SET_DIR_IN:
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO9564DIR));
		tmp &= ~cpu_to_le32(arg);
		*(volatile u32 *)(RALINK_REG_PIO9564DIR) = tmp;
		break;
	case RALINK_GPIO9564_SET_DIR_OUT:
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO9564DIR));
		tmp |= cpu_to_le32(arg);
		*(volatile u32 *)(RALINK_REG_PIO9564DIR) = tmp;
		break;
	case RALINK_GPIO9564_READ:
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO9564DATA));
		put_user(tmp, (int __user *)arg);
		break;
	case RALINK_GPIO9564_WRITE:
		*(volatile u32 *)(RALINK_REG_PIO9564DATA) = cpu_to_le32(arg);
		break;
	case RALINK_GPIO9564_SET:
		*(volatile u32 *)(RALINK_REG_PIO9564SET) = cpu_to_le32(arg);
		break;
	case RALINK_GPIO9564_CLEAR:
		*(volatile u32 *)(RALINK_REG_PIO9564SET) = cpu_to_le32(arg);
		break;
#elif defined (RALINK_GPIO_HAS_4524)
	case RALINK_GPIO3924_SET_DIR:
		*(volatile u32 *)(RALINK_REG_PIO3924DIR) = cpu_to_le32(arg);
		break;
	case RALINK_GPIO3924_SET_DIR_IN:
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO3924DIR));
		tmp &= ~cpu_to_le32(arg);
		*(volatile u32 *)(RALINK_REG_PIO3924DIR) = tmp;
		break;
	case RALINK_GPIO3924_SET_DIR_OUT:
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO3924DIR));
		tmp |= cpu_to_le32(arg);
		*(volatile u32 *)(RALINK_REG_PIO3924DIR) = tmp;
		break;
	case RALINK_GPIO3924_READ:
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO3924DATA));
		put_user(tmp, (int __user *)arg);
		break;
	case RALINK_GPIO3924_WRITE:
		*(volatile u32 *)(RALINK_REG_PIO3924DATA) = cpu_to_le32(arg);
		break;
	case RALINK_GPIO3924_SET:
		*(volatile u32 *)(RALINK_REG_PIO3924SET) = cpu_to_le32(arg);
		break;
	case RALINK_GPIO3924_CLEAR:
		*(volatile u32 *)(RALINK_REG_PIO3924SET) = cpu_to_le32(arg);
		break;

	case RALINK_GPIO4540_SET_DIR:
		*(volatile u32 *)(RALINK_REG_PIO4540DIR) = cpu_to_le32(arg);
		break;
	case RALINK_GPIO4540_SET_DIR_IN:
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO4540DIR));
		tmp &= ~cpu_to_le32(arg);
		*(volatile u32 *)(RALINK_REG_PIO4540DIR) = tmp;
		break;
	case RALINK_GPIO4540_SET_DIR_OUT:
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO4540DIR));
		tmp |= cpu_to_le32(arg);
		*(volatile u32 *)(RALINK_REG_PIO4540DIR) = tmp;
		break;
	case RALINK_GPIO4540_READ:
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO4540DATA));
		put_user(tmp, (int __user *)arg);
		break;
	case RALINK_GPIO4540_WRITE:
		*(volatile u32 *)(RALINK_REG_PIO4540DATA) = cpu_to_le32(arg);
		break;
	case RALINK_GPIO4540_SET:
		*(volatile u32 *)(RALINK_REG_PIO4540SET) = cpu_to_le32(arg);
		break;
	case RALINK_GPIO4540_CLEAR:
		*(volatile u32 *)(RALINK_REG_PIO4540SET) = cpu_to_le32(arg);
		break;
#elif defined (RALINK_GPIO_HAS_5124)
	case RALINK_GPIO3924_SET_DIR:
		*(volatile u32 *)(RALINK_REG_PIO3924DIR) = cpu_to_le32(arg);
		break;
	case RALINK_GPIO3924_SET_DIR_IN:
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO3924DIR));
		tmp &= ~cpu_to_le32(arg);
		*(volatile u32 *)(RALINK_REG_PIO3924DIR) = tmp;
		break;
	case RALINK_GPIO3924_SET_DIR_OUT:
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO3924DIR));
		tmp |= cpu_to_le32(arg);
		*(volatile u32 *)(RALINK_REG_PIO3924DIR) = tmp;
		break;
	case RALINK_GPIO3924_READ:
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO3924DATA));
		put_user(tmp, (int __user *)arg);
		break;
	case RALINK_GPIO3924_WRITE:
		*(volatile u32 *)(RALINK_REG_PIO3924DATA) = cpu_to_le32(arg);
		break;
	case RALINK_GPIO3924_SET:
		*(volatile u32 *)(RALINK_REG_PIO3924SET) = cpu_to_le32(arg);
		break;
	case RALINK_GPIO3924_CLEAR:
		*(volatile u32 *)(RALINK_REG_PIO3924SET) = cpu_to_le32(arg);
		break;

	case RALINK_GPIO5140_SET_DIR:
		*(volatile u32 *)(RALINK_REG_PIO5140DIR) = cpu_to_le32(arg);
		break;
	case RALINK_GPIO5140_SET_DIR_IN:
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO5140DIR));
		tmp &= ~cpu_to_le32(arg);
		*(volatile u32 *)(RALINK_REG_PIO5140DIR) = tmp;
		break;
	case RALINK_GPIO5140_SET_DIR_OUT:
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO5140DIR));
		tmp |= cpu_to_le32(arg);
		*(volatile u32 *)(RALINK_REG_PIO5140DIR) = tmp;
		break;
	case RALINK_GPIO5140_READ:
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO5140DATA));
		put_user(tmp, (int __user *)arg);
		break;
	case RALINK_GPIO5140_WRITE:
		*(volatile u32 *)(RALINK_REG_PIO5140DATA) = cpu_to_le32(arg);
		break;
	case RALINK_GPIO5140_SET:
		*(volatile u32 *)(RALINK_REG_PIO5140SET) = cpu_to_le32(arg);
		break;
	case RALINK_GPIO5140_CLEAR:
		*(volatile u32 *)(RALINK_REG_PIO5140SET) = cpu_to_le32(arg);
		break;
#elif defined (RALINK_GPIO_HAS_9524) || defined (RALINK_GPIO_HAS_7224)
	case RALINK_GPIO3924_SET_DIR:
		*(volatile u32 *)(RALINK_REG_PIO3924DIR) = cpu_to_le32(arg);
		break;
	case RALINK_GPIO3924_SET_DIR_IN:
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO3924DIR));
		tmp &= ~cpu_to_le32(arg);
		*(volatile u32 *)(RALINK_REG_PIO3924DIR) = tmp;
		break;
	case RALINK_GPIO3924_SET_DIR_OUT:
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO3924DIR));
		tmp |= cpu_to_le32(arg);
		*(volatile u32 *)(RALINK_REG_PIO3924DIR) = tmp;
		break;
	case RALINK_GPIO3924_READ:
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO3924DATA));
		put_user(tmp, (int __user *)arg);
		break;
	case RALINK_GPIO3924_WRITE:
		*(volatile u32 *)(RALINK_REG_PIO3924DATA) = cpu_to_le32(arg);
		break;
	case RALINK_GPIO3924_SET:
		*(volatile u32 *)(RALINK_REG_PIO3924SET) = cpu_to_le32(arg);
		break;
	case RALINK_GPIO3924_CLEAR:
		*(volatile u32 *)(RALINK_REG_PIO3924SET) = cpu_to_le32(arg);
		break;

	case RALINK_GPIO7140_SET_DIR:
		*(volatile u32 *)(RALINK_REG_PIO7140DIR) = cpu_to_le32(arg);
		break;
	case RALINK_GPIO7140_SET_DIR_IN:
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO7140DIR));
		tmp &= ~cpu_to_le32(arg);
		*(volatile u32 *)(RALINK_REG_PIO7140DIR) = tmp;
		break;
	case RALINK_GPIO7140_SET_DIR_OUT:
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO7140DIR));
		tmp |= cpu_to_le32(arg);
		*(volatile u32 *)(RALINK_REG_PIO7140DIR) = tmp;
		break;
	case RALINK_GPIO7140_READ:
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO7140DATA));
		put_user(tmp, (int __user *)arg);
		break;
	case RALINK_GPIO7140_WRITE:
		*(volatile u32 *)(RALINK_REG_PIO7140DATA) = cpu_to_le32(arg);
		break;
	case RALINK_GPIO7140_SET:
		*(volatile u32 *)(RALINK_REG_PIO7140SET) = cpu_to_le32(arg);
		break;
	case RALINK_GPIO7140_CLEAR:
		*(volatile u32 *)(RALINK_REG_PIO7140SET) = cpu_to_le32(arg);
		break;
#if defined (RALINK_GPIO_HAS_7224)
	case RALINK_GPIO72_SET_DIR:
		*(volatile u32 *)(RALINK_REG_PIO72DIR) = cpu_to_le32(arg);
		break;
	case RALINK_GPIO72_SET_DIR_IN:
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO72DIR));
		tmp &= ~cpu_to_le32(arg);
		*(volatile u32 *)(RALINK_REG_PIO72DIR) = tmp;
		break;
	case RALINK_GPIO72_SET_DIR_OUT:
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO72DIR));
		tmp |= cpu_to_le32(arg);
		*(volatile u32 *)(RALINK_REG_PIO72DIR) = tmp;
		break;
	case RALINK_GPIO72_READ:
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO72DATA));
		put_user(tmp, (int __user *)arg);
		break;
	case RALINK_GPIO72_WRITE:
		*(volatile u32 *)(RALINK_REG_PIO72DATA) = cpu_to_le32(arg);
		break;
	case RALINK_GPIO72_SET:
		*(volatile u32 *)(RALINK_REG_PIO72SET) = cpu_to_le32(arg);
		break;
	case RALINK_GPIO72_CLEAR:
		*(volatile u32 *)(RALINK_REG_PIO72SET) = cpu_to_le32(arg);
		break;
#else
	case RALINK_GPIO9572_SET_DIR:
		*(volatile u32 *)(RALINK_REG_PIO9572DIR) = cpu_to_le32(arg);
		break;
	case RALINK_GPIO9572_SET_DIR_IN:
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO9572DIR));
		tmp &= ~cpu_to_le32(arg);
		*(volatile u32 *)(RALINK_REG_PIO9572DIR) = tmp;
		break;
	case RALINK_GPIO9572_SET_DIR_OUT:
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO9572DIR));
		tmp |= cpu_to_le32(arg);
		*(volatile u32 *)(RALINK_REG_PIO9572DIR) = tmp;
		break;
	case RALINK_GPIO9572_READ:
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO9572DATA));
		put_user(tmp, (int __user *)arg);
		break;
	case RALINK_GPIO9572_WRITE:
		*(volatile u32 *)(RALINK_REG_PIO9572DATA) = cpu_to_le32(arg);
		break;
	case RALINK_GPIO9572_SET:
		*(volatile u32 *)(RALINK_REG_PIO9572SET) = cpu_to_le32(arg);
		break;
	case RALINK_GPIO9572_CLEAR:
		*(volatile u32 *)(RALINK_REG_PIO9572SET) = cpu_to_le32(arg);
		break;
#endif
#endif

	case RALINK_GPIO_LED_SET:
#ifdef CONFIG_RALINK_GPIO_LED
		copy_from_user(&led, (ralink_gpio_led_info *)arg, sizeof(led));
		ralink_gpio_led_set(led);
#else
		printk(KERN_ERR NAME ": gpio led support not built\n");
#endif
		break;
	default:
		return -ENOIOCTLCMD;
	}
	return 0;
}

int ralink_gpio_open(struct inode *inode, struct file *file)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
	MOD_INC_USE_COUNT;
#else
	try_module_get(THIS_MODULE);
#endif
    INIT_WORK(&gpio_event_hold, gpio_hold_notify);
    INIT_WORK(&gpio_event_click, gpio_click_notify);
	return 0;
}

int ralink_gpio_release(struct inode *inode, struct file *file)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
	MOD_DEC_USE_COUNT;
#else
	module_put(THIS_MODULE);
#endif
	return 0;
}

struct file_operations ralink_gpio_fops =
{
	owner:		THIS_MODULE,
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,35)
	unlocked_ioctl:	ralink_gpio_ioctl,
#else
	ioctl:		ralink_gpio_ioctl,
#endif
	open:		ralink_gpio_open,
	release:	ralink_gpio_release,
};

#ifdef CONFIG_RALINK_GPIO_LED

#if RALINK_GPIO_LED_LOW_ACT

#define __LED_ON(gpio)      ra_gpio_led_clr |= RALINK_GPIO(gpio);
#define __LED_OFF(gpio)     ra_gpio_led_set |= RALINK_GPIO(gpio);
#define __LED2722_ON(gpio)  ra_gpio2722_led_clr |= RALINK_GPIO((gpio-22));
#define __LED2722_OFF(gpio) ra_gpio2722_led_set |= RALINK_GPIO((gpio-22));
#define __LED3924_ON(gpio)  ra_gpio3924_led_clr |= RALINK_GPIO((gpio-24));
#define __LED3924_OFF(gpio) ra_gpio3924_led_set |= RALINK_GPIO((gpio-24));
#define __LED4540_ON(gpio)  ra_gpio4540_led_clr |= RALINK_GPIO((gpio-40));
#define __LED4540_OFF(gpio) ra_gpio4540_led_set |= RALINK_GPIO((gpio-40));
#define __LED5140_ON(gpio)  ra_gpio5140_led_clr |= RALINK_GPIO((gpio-40));
#define __LED5140_OFF(gpio) ra_gpio5140_led_set |= RALINK_GPIO((gpio-40));
#define __LED7140_ON(gpio)  ra_gpio7140_led_clr |= RALINK_GPIO((gpio-40));
#define __LED7140_OFF(gpio) ra_gpio7140_led_set |= RALINK_GPIO((gpio-40));

#if defined (RALINK_GPIO_HAS_7224)
#define __LED72_ON(gpio)  ra_gpio72_led_clr |= RALINK_GPIO((gpio-72));
#define __LED72_OFF(gpio) ra_gpio72_led_set |= RALINK_GPIO((gpio-72));
#else
#define __LED9572_ON(gpio)  ra_gpio9572_led_clr |= RALINK_GPIO((gpio-72));
#define __LED9572_OFF(gpio) ra_gpio9572_led_set |= RALINK_GPIO((gpio-72));
#endif

#if defined (RALINK_GPIO_HAS_9532)
#define __LED6332_ON(gpio)  ra_gpio6332_led_clr |= RALINK_GPIO((gpio-32));
#define __LED6332_OFF(gpio) ra_gpio6332_led_set |= RALINK_GPIO((gpio-32));
#define __LED9564_ON(gpio)  ra_gpio9564_led_clr |= RALINK_GPIO((gpio-64));
#define __LED9564_OFF(gpio) ra_gpio9564_led_set |= RALINK_GPIO((gpio-64));
#endif

#else

#define __LED_ON(gpio)      ra_gpio_led_set |= RALINK_GPIO(gpio);
#define __LED_OFF(gpio)     ra_gpio_led_clr |= RALINK_GPIO(gpio);
#define __LED2722_ON(gpio)  ra_gpio2722_led_set |= RALINK_GPIO((gpio-22));
#define __LED2722_OFF(gpio) ra_gpio2722_led_clr |= RALINK_GPIO((gpio-22));
#define __LED3924_ON(gpio)  ra_gpio3924_led_set |= RALINK_GPIO((gpio-24));
#define __LED3924_OFF(gpio) ra_gpio3924_led_clr |= RALINK_GPIO((gpio-24));
#define __LED4540_ON(gpio)  ra_gpio4540_led_set |= RALINK_GPIO((gpio-40));
#define __LED4540_OFF(gpio) ra_gpio4540_led_clr |= RALINK_GPIO((gpio-40));
#define __LED5140_ON(gpio)  ra_gpio5140_led_set |= RALINK_GPIO((gpio-40));
#define __LED5140_OFF(gpio) ra_gpio5140_led_clr |= RALINK_GPIO((gpio-40));
#define __LED7140_ON(gpio)  ra_gpio7140_led_set |= RALINK_GPIO((gpio-40));
#define __LED7140_OFF(gpio) ra_gpio7140_led_clr |= RALINK_GPIO((gpio-40));
#if defined (RALINK_GPIO_HAS_7224)
#define __LED72_ON(gpio)  ra_gpio72_led_set |= RALINK_GPIO((gpio-72));
#define __LED72_OFF(gpio) ra_gpio72_led_clr |= RALINK_GPIO((gpio-72));
#else
#define __LED9572_ON(gpio)  ra_gpio9572_led_set |= RALINK_GPIO((gpio-72));
#define __LED9572_OFF(gpio) ra_gpio9572_led_clr |= RALINK_GPIO((gpio-72));
#endif

#if defined (RALINK_GPIO_HAS_9532)
#define __LED6332_ON(gpio)  ra_gpio6332_led_set |= RALINK_GPIO((gpio-32));
#define __LED6332_OFF(gpio) ra_gpio6332_led_clr |= RALINK_GPIO((gpio-32));
#define __LED9564_ON(gpio)  ra_gpio9564_led_set |= RALINK_GPIO((gpio-64));
#define __LED9564_OFF(gpio) ra_gpio9564_led_clr |= RALINK_GPIO((gpio-64));
#endif


#endif

static void ralink_gpio_led_do_timer(unsigned long unused)
{
	int i;
	unsigned int x;

#if defined (RALINK_GPIO_HAS_2722)
	for (i = 0; i < 22; i++) {
		ralink_gpio_led_stat[i].ticks++;
		if (ralink_gpio_led_data[i].gpio == -1) //-1 means unused
			continue;
		if (ralink_gpio_led_data[i].on == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].off == 0) { //always on
			__LED_ON(i);
			continue;
		}
		if (ralink_gpio_led_data[i].off == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].rests == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].on == 0 ||
				ralink_gpio_led_data[i].blinks == 0 ||
				ralink_gpio_led_data[i].times == 0) { //always off
			__LED_OFF(i);
			continue;
		}

		//led turn on or off
		if (ralink_gpio_led_data[i].blinks == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].rests == 0) { //always blinking
			x = ralink_gpio_led_stat[i].ticks % (ralink_gpio_led_data[i].on
					+ ralink_gpio_led_data[i].off);
		}
		else {
			unsigned int a, b, c, d, o, t;
			a = ralink_gpio_led_data[i].blinks / 2;
			b = ralink_gpio_led_data[i].rests / 2;
			c = ralink_gpio_led_data[i].blinks % 2;
			d = ralink_gpio_led_data[i].rests % 2;
			o = ralink_gpio_led_data[i].on + ralink_gpio_led_data[i].off;
			//t = blinking ticks
			t = a * o + ralink_gpio_led_data[i].on * c;
			//x = ticks % (blinking ticks + resting ticks)
			x = ralink_gpio_led_stat[i].ticks %
				(t + b * o + ralink_gpio_led_data[i].on * d);
			//starts from 0 at resting cycles
			if (x >= t)
				x -= t;
			x %= o;
		}
		if (x < ralink_gpio_led_data[i].on) {
			__LED_ON(i);
			if (ralink_gpio_led_stat[i].ticks && x == 0)
				ralink_gpio_led_stat[i].offs++;
#if RALINK_LED_DEBUG
			printk("t%d gpio%d on,", ralink_gpio_led_stat[i].ticks, i);
#endif
		}
		else {
			__LED_OFF(i);
			if (x == ralink_gpio_led_data[i].on)
				ralink_gpio_led_stat[i].ons++;
#if RALINK_LED_DEBUG
			printk("t%d gpio%d off,", ralink_gpio_led_stat[i].ticks, i);
#endif
		}

		//blinking or resting
		if (ralink_gpio_led_data[i].blinks == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].rests == 0) { //always blinking
			continue;
		}
		else {
			x = ralink_gpio_led_stat[i].ons + ralink_gpio_led_stat[i].offs;
			if (!ralink_gpio_led_stat[i].resting) {
				if (x == ralink_gpio_led_data[i].blinks) {
					ralink_gpio_led_stat[i].resting = 1;
					ralink_gpio_led_stat[i].ons = 0;
					ralink_gpio_led_stat[i].offs = 0;
					ralink_gpio_led_stat[i].times++;
				}
			}
			else {
				if (x == ralink_gpio_led_data[i].rests) {
					ralink_gpio_led_stat[i].resting = 0;
					ralink_gpio_led_stat[i].ons = 0;
					ralink_gpio_led_stat[i].offs = 0;
				}
			}
		}
		if (ralink_gpio_led_stat[i].resting) {
			__LED_OFF(i);
#if RALINK_LED_DEBUG
			printk("resting,");
		} else {
			printk("blinking,");
#endif
		}

		//number of times
		if (ralink_gpio_led_data[i].times != RALINK_GPIO_LED_INFINITY)
		{
			if (ralink_gpio_led_stat[i].times ==
					ralink_gpio_led_data[i].times) {
				__LED_OFF(i);
				ralink_gpio_led_data[i].gpio = -1; //stop
			}
#if RALINK_LED_DEBUG
			printk("T%d\n", ralink_gpio_led_stat[i].times);
		} else {
			printk("T@\n");
#endif
		}
	}

	for (i = 22; i <  RALINK_GPIO_NUMBER; i++) {
		ralink_gpio_led_stat[i].ticks++;
		if (ralink_gpio_led_data[i].gpio == -1) //-1 means unused
			continue;
		if (ralink_gpio_led_data[i].on == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].off == 0) { //always on
			__LED2722_ON(i);
			continue;
		}
		if (ralink_gpio_led_data[i].off == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].rests == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].on == 0 ||
				ralink_gpio_led_data[i].blinks == 0 ||
				ralink_gpio_led_data[i].times == 0) { //always off
			__LED2722_OFF(i);
			continue;
		}

		//led turn on or off
		if (ralink_gpio_led_data[i].blinks == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].rests == 0) { //always blinking
			x = ralink_gpio_led_stat[i].ticks % (ralink_gpio_led_data[i].on
					+ ralink_gpio_led_data[i].off);
		}
		else {
			unsigned int a, b, c, d, o, t;
			a = ralink_gpio_led_data[i].blinks / 2;
			b = ralink_gpio_led_data[i].rests / 2;
			c = ralink_gpio_led_data[i].blinks % 2;
			d = ralink_gpio_led_data[i].rests % 2;
			o = ralink_gpio_led_data[i].on + ralink_gpio_led_data[i].off;
			//t = blinking ticks
			t = a * o + ralink_gpio_led_data[i].on * c;
			//x = ticks % (blinking ticks + resting ticks)
			x = ralink_gpio_led_stat[i].ticks %
				(t + b * o + ralink_gpio_led_data[i].on * d);
			//starts from 0 at resting cycles
			if (x >= t)
				x -= t;
			x %= o;
		}
		if (x < ralink_gpio_led_data[i].on) {
			__LED2722_ON(i);
			if (ralink_gpio_led_stat[i].ticks && x == 0)
				ralink_gpio_led_stat[i].offs++;
#if RALINK_LED_DEBUG
			printk("t%d gpio%d on,", ralink_gpio_led_stat[i].ticks, i);
#endif
		}
		else {
			__LED2722_OFF(i);
			if (x == ralink_gpio_led_data[i].on)
				ralink_gpio_led_stat[i].ons++;
#if RALINK_LED_DEBUG
			printk("t%d gpio%d off,", ralink_gpio_led_stat[i].ticks, i);
#endif
		}

		//blinking or resting
		if (ralink_gpio_led_data[i].blinks == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].rests == 0) { //always blinking
			continue;
		}
		else {
			x = ralink_gpio_led_stat[i].ons + ralink_gpio_led_stat[i].offs;
			if (!ralink_gpio_led_stat[i].resting) {
				if (x == ralink_gpio_led_data[i].blinks) {
					ralink_gpio_led_stat[i].resting = 1;
					ralink_gpio_led_stat[i].ons = 0;
					ralink_gpio_led_stat[i].offs = 0;
					ralink_gpio_led_stat[i].times++;
				}
			}
			else {
				if (x == ralink_gpio_led_data[i].rests) {
					ralink_gpio_led_stat[i].resting = 0;
					ralink_gpio_led_stat[i].ons = 0;
					ralink_gpio_led_stat[i].offs = 0;
				}
			}
		}
		if (ralink_gpio_led_stat[i].resting) {
			__LED2722_OFF(i);
#if RALINK_LED_DEBUG
			printk("resting,");
		} else {
			printk("blinking,");
#endif
		}

		//number of times
		if (ralink_gpio_led_data[i].times != RALINK_GPIO_LED_INFINITY)
		{
			if (ralink_gpio_led_stat[i].times ==
					ralink_gpio_led_data[i].times) {
				__LED2722_OFF(i);
				ralink_gpio_led_data[i].gpio = -1; //stop
			}
#if RALINK_LED_DEBUG
			printk("T%d\n", ralink_gpio_led_stat[i].times);
		} else {
			printk("T@\n");
#endif
		}
	}
#else
	#if defined (RALINK_GPIO_HAS_9532)
	for (i = 0; i < 31; i++) {
		ralink_gpio_led_stat[i].ticks++;
		if (ralink_gpio_led_data[i].gpio == -1){ //-1 means unused	
			continue;
		}
		if (ralink_gpio_led_data[i].on == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].off == 0) { //always on
			__LED_ON(i);	
			continue;
		}
		if (ralink_gpio_led_data[i].off == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].rests == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].on == 0 ||
				ralink_gpio_led_data[i].blinks == 0 ||
				ralink_gpio_led_data[i].times == 0) { //always off
			__LED_OFF(i);	
			continue;
		}	
		//led turn on or off
		if (ralink_gpio_led_data[i].blinks == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].rests == 0) { //always blinking
			x = ralink_gpio_led_stat[i].ticks % (ralink_gpio_led_data[i].on
					+ ralink_gpio_led_data[i].off);
		}
		else {
			unsigned int a, b, c, d, o, t;
			a = ralink_gpio_led_data[i].blinks / 2;
			b = ralink_gpio_led_data[i].rests / 2;
			c = ralink_gpio_led_data[i].blinks % 2;
			d = ralink_gpio_led_data[i].rests % 2;
			o = ralink_gpio_led_data[i].on + ralink_gpio_led_data[i].off;
			//t = blinking ticks
			t = a * o + ralink_gpio_led_data[i].on * c;
			//x = ticks % (blinking ticks + resting ticks)
			x = ralink_gpio_led_stat[i].ticks %
				(t + b * o + ralink_gpio_led_data[i].on * d);
			//starts from 0 at resting cycles
			if (x >= t)
				x -= t;
			x %= o;
		}
		if (x < ralink_gpio_led_data[i].on) {
			__LED_ON(i);
			if (ralink_gpio_led_stat[i].ticks && x == 0)
				ralink_gpio_led_stat[i].offs++;
#if RALINK_LED_DEBUG
			printk("t%d gpio%d on,", ralink_gpio_led_stat[i].ticks, i);
#endif
		}
		else {
			__LED_OFF(i);
			if (x == ralink_gpio_led_data[i].on)
				ralink_gpio_led_stat[i].ons++;
#if RALINK_LED_DEBUG
			printk("t%d gpio%d off,", ralink_gpio_led_stat[i].ticks, i);
#endif
		}

		//blinking or resting
		if (ralink_gpio_led_data[i].blinks == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].rests == 0) { //always blinking
			continue;
		}
		else {
			x = ralink_gpio_led_stat[i].ons + ralink_gpio_led_stat[i].offs;
			if (!ralink_gpio_led_stat[i].resting) {
				if (x == ralink_gpio_led_data[i].blinks) {
					ralink_gpio_led_stat[i].resting = 1;
					ralink_gpio_led_stat[i].ons = 0;
					ralink_gpio_led_stat[i].offs = 0;
					ralink_gpio_led_stat[i].times++;
				}
			}
			else {
				if (x == ralink_gpio_led_data[i].rests) {
					ralink_gpio_led_stat[i].resting = 0;
					ralink_gpio_led_stat[i].ons = 0;
					ralink_gpio_led_stat[i].offs = 0;
				}
			}
		}
		if (ralink_gpio_led_stat[i].resting) {
			__LED_OFF(i);
#if RALINK_LED_DEBUG
			printk("resting,");
		} else {
			printk("blinking,");
#endif
		}

		//number of times
		if (ralink_gpio_led_data[i].times != RALINK_GPIO_LED_INFINITY)
		{
			if (ralink_gpio_led_stat[i].times ==
					ralink_gpio_led_data[i].times) {
				__LED_OFF(i);
				ralink_gpio_led_data[i].gpio = -1; //stop
			}
#if RALINK_LED_DEBUG
			printk("T%d\n", ralink_gpio_led_stat[i].times);
		} else {
			printk("T@\n");
#endif
		}
	}	
	
	
	#else
	for (i = 0; i < 24; i++) {
		ralink_gpio_led_stat[i].ticks++;
		if (ralink_gpio_led_data[i].gpio == -1) //-1 means unused
			continue;
		if (ralink_gpio_led_data[i].on == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].off == 0) { //always on
			__LED_ON(i);
			continue;
		}
		if (ralink_gpio_led_data[i].off == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].rests == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].on == 0 ||
				ralink_gpio_led_data[i].blinks == 0 ||
				ralink_gpio_led_data[i].times == 0) { //always off
			__LED_OFF(i);
			continue;
		}

		//led turn on or off
		if (ralink_gpio_led_data[i].blinks == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].rests == 0) { //always blinking
			x = ralink_gpio_led_stat[i].ticks % (ralink_gpio_led_data[i].on
					+ ralink_gpio_led_data[i].off);
		}
		else {
			unsigned int a, b, c, d, o, t;
			a = ralink_gpio_led_data[i].blinks / 2;
			b = ralink_gpio_led_data[i].rests / 2;
			c = ralink_gpio_led_data[i].blinks % 2;
			d = ralink_gpio_led_data[i].rests % 2;
			o = ralink_gpio_led_data[i].on + ralink_gpio_led_data[i].off;
			//t = blinking ticks
			t = a * o + ralink_gpio_led_data[i].on * c;
			//x = ticks % (blinking ticks + resting ticks)
			x = ralink_gpio_led_stat[i].ticks %
				(t + b * o + ralink_gpio_led_data[i].on * d);
			//starts from 0 at resting cycles
			if (x >= t)
				x -= t;
			x %= o;
		}
		if (x < ralink_gpio_led_data[i].on) {
			__LED_ON(i);
			if (ralink_gpio_led_stat[i].ticks && x == 0)
				ralink_gpio_led_stat[i].offs++;
#if RALINK_LED_DEBUG
			printk("t%d gpio%d on,", ralink_gpio_led_stat[i].ticks, i);
#endif
		}
		else {
			__LED_OFF(i);
			if (x == ralink_gpio_led_data[i].on)
				ralink_gpio_led_stat[i].ons++;
#if RALINK_LED_DEBUG
			printk("t%d gpio%d off,", ralink_gpio_led_stat[i].ticks, i);
#endif
		}

		//blinking or resting
		if (ralink_gpio_led_data[i].blinks == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].rests == 0) { //always blinking
			continue;
		}
		else {
			x = ralink_gpio_led_stat[i].ons + ralink_gpio_led_stat[i].offs;
			if (!ralink_gpio_led_stat[i].resting) {
				if (x == ralink_gpio_led_data[i].blinks) {
					ralink_gpio_led_stat[i].resting = 1;
					ralink_gpio_led_stat[i].ons = 0;
					ralink_gpio_led_stat[i].offs = 0;
					ralink_gpio_led_stat[i].times++;
				}
			}
			else {
				if (x == ralink_gpio_led_data[i].rests) {
					ralink_gpio_led_stat[i].resting = 0;
					ralink_gpio_led_stat[i].ons = 0;
					ralink_gpio_led_stat[i].offs = 0;
				}
			}
		}
		if (ralink_gpio_led_stat[i].resting) {
			__LED_OFF(i);
#if RALINK_LED_DEBUG
			printk("resting,");
		} else {
			printk("blinking,");
#endif
		}

		//number of times
		if (ralink_gpio_led_data[i].times != RALINK_GPIO_LED_INFINITY)
		{
			if (ralink_gpio_led_stat[i].times ==
					ralink_gpio_led_data[i].times) {
				__LED_OFF(i);
				ralink_gpio_led_data[i].gpio = -1; //stop
			}
#if RALINK_LED_DEBUG
			printk("T%d\n", ralink_gpio_led_stat[i].times);
		} else {
			printk("T@\n");
#endif
		}
	}
#endif
#if defined (RALINK_GPIO_HAS_4524)
	for (i = 24; i < 40; i++) {
		ralink_gpio_led_stat[i].ticks++;
		if (ralink_gpio_led_data[i].gpio == -1) //-1 means unused
			continue;
		if (ralink_gpio_led_data[i].on == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].off == 0) { //always on
			__LED3924_ON(i);
			continue;
		}
		if (ralink_gpio_led_data[i].off == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].rests == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].on == 0 ||
				ralink_gpio_led_data[i].blinks == 0 ||
				ralink_gpio_led_data[i].times == 0) { //always off
			__LED3924_OFF(i);
			continue;
		}

		//led turn on or off
		if (ralink_gpio_led_data[i].blinks == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].rests == 0) { //always blinking
			x = ralink_gpio_led_stat[i].ticks % (ralink_gpio_led_data[i].on
					+ ralink_gpio_led_data[i].off);
		}
		else {
			unsigned int a, b, c, d, o, t;
			a = ralink_gpio_led_data[i].blinks / 2;
			b = ralink_gpio_led_data[i].rests / 2;
			c = ralink_gpio_led_data[i].blinks % 2;
			d = ralink_gpio_led_data[i].rests % 2;
			o = ralink_gpio_led_data[i].on + ralink_gpio_led_data[i].off;
			//t = blinking ticks
			t = a * o + ralink_gpio_led_data[i].on * c;
			//x = ticks % (blinking ticks + resting ticks)
			x = ralink_gpio_led_stat[i].ticks %
				(t + b * o + ralink_gpio_led_data[i].on * d);
			//starts from 0 at resting cycles
			if (x >= t)
				x -= t;
			x %= o;
		}
		if (x < ralink_gpio_led_data[i].on) {
			__LED3924_ON(i);
			if (ralink_gpio_led_stat[i].ticks && x == 0)
				ralink_gpio_led_stat[i].offs++;
#if RALINK_LED_DEBUG
			printk("t%d gpio%d on,", ralink_gpio_led_stat[i].ticks, i);
#endif
		}
		else {
			__LED3924_OFF(i);
			if (x == ralink_gpio_led_data[i].on)
				ralink_gpio_led_stat[i].ons++;
#if RALINK_LED_DEBUG
			printk("t%d gpio%d off,", ralink_gpio_led_stat[i].ticks, i);
#endif
		}

		//blinking or resting
		if (ralink_gpio_led_data[i].blinks == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].rests == 0) { //always blinking
			continue;
		}
		else {
			x = ralink_gpio_led_stat[i].ons + ralink_gpio_led_stat[i].offs;
			if (!ralink_gpio_led_stat[i].resting) {
				if (x == ralink_gpio_led_data[i].blinks) {
					ralink_gpio_led_stat[i].resting = 1;
					ralink_gpio_led_stat[i].ons = 0;
					ralink_gpio_led_stat[i].offs = 0;
					ralink_gpio_led_stat[i].times++;
				}
			}
			else {
				if (x == ralink_gpio_led_data[i].rests) {
					ralink_gpio_led_stat[i].resting = 0;
					ralink_gpio_led_stat[i].ons = 0;
					ralink_gpio_led_stat[i].offs = 0;
				}
			}
		}
		if (ralink_gpio_led_stat[i].resting) {
			__LED3924_OFF(i);
#if RALINK_LED_DEBUG
			printk("resting,");
		} else {
			printk("blinking,");
#endif
		}

		//number of times
		if (ralink_gpio_led_data[i].times != RALINK_GPIO_LED_INFINITY)
		{
			if (ralink_gpio_led_stat[i].times ==
					ralink_gpio_led_data[i].times) {
				__LED3924_OFF(i);
				ralink_gpio_led_data[i].gpio = -1; //stop
			}
#if RALINK_LED_DEBUG
			printk("T%d\n", ralink_gpio_led_stat[i].times);
		} else {
			printk("T@\n");
#endif
		}
	}

	for (i = 40; i <  RALINK_GPIO_NUMBER; i++) {
		ralink_gpio_led_stat[i].ticks++;
		if (ralink_gpio_led_data[i].gpio == -1) //-1 means unused
			continue;
		if (ralink_gpio_led_data[i].on == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].off == 0) { //always on
			__LED4540_ON(i);
			continue;
		}
		if (ralink_gpio_led_data[i].off == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].rests == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].on == 0 ||
				ralink_gpio_led_data[i].blinks == 0 ||
				ralink_gpio_led_data[i].times == 0) { //always off
			__LED4540_OFF(i);
			continue;
		}

		//led turn on or off
		if (ralink_gpio_led_data[i].blinks == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].rests == 0) { //always blinking
			x = ralink_gpio_led_stat[i].ticks % (ralink_gpio_led_data[i].on
					+ ralink_gpio_led_data[i].off);
		}
		else {
			unsigned int a, b, c, d, o, t;
			a = ralink_gpio_led_data[i].blinks / 2;
			b = ralink_gpio_led_data[i].rests / 2;
			c = ralink_gpio_led_data[i].blinks % 2;
			d = ralink_gpio_led_data[i].rests % 2;
			o = ralink_gpio_led_data[i].on + ralink_gpio_led_data[i].off;
			//t = blinking ticks
			t = a * o + ralink_gpio_led_data[i].on * c;
			//x = ticks % (blinking ticks + resting ticks)
			x = ralink_gpio_led_stat[i].ticks %
				(t + b * o + ralink_gpio_led_data[i].on * d);
			//starts from 0 at resting cycles
			if (x >= t)
				x -= t;
			x %= o;
		}
		if (x < ralink_gpio_led_data[i].on) {
			__LED4540_ON(i);
			if (ralink_gpio_led_stat[i].ticks && x == 0)
				ralink_gpio_led_stat[i].offs++;
#if RALINK_LED_DEBUG
			printk("t%d gpio%d on,", ralink_gpio_led_stat[i].ticks, i);
#endif
		}
		else {
			__LED4540_OFF(i);
			if (x == ralink_gpio_led_data[i].on)
				ralink_gpio_led_stat[i].ons++;
#if RALINK_LED_DEBUG
			printk("t%d gpio%d off,", ralink_gpio_led_stat[i].ticks, i);
#endif
		}

		//blinking or resting
		if (ralink_gpio_led_data[i].blinks == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].rests == 0) { //always blinking
			continue;
		}
		else {
			x = ralink_gpio_led_stat[i].ons + ralink_gpio_led_stat[i].offs;
			if (!ralink_gpio_led_stat[i].resting) {
				if (x == ralink_gpio_led_data[i].blinks) {
					ralink_gpio_led_stat[i].resting = 1;
					ralink_gpio_led_stat[i].ons = 0;
					ralink_gpio_led_stat[i].offs = 0;
					ralink_gpio_led_stat[i].times++;
				}
			}
			else {
				if (x == ralink_gpio_led_data[i].rests) {
					ralink_gpio_led_stat[i].resting = 0;
					ralink_gpio_led_stat[i].ons = 0;
					ralink_gpio_led_stat[i].offs = 0;
				}
			}
		}
		if (ralink_gpio_led_stat[i].resting) {
			__LED4540_OFF(i);
#if RALINK_LED_DEBUG
			printk("resting,");
		} else {
			printk("blinking,");
#endif
		}

		//number of times
		if (ralink_gpio_led_data[i].times != RALINK_GPIO_LED_INFINITY)
		{
			if (ralink_gpio_led_stat[i].times ==
					ralink_gpio_led_data[i].times) {
				__LED4540_OFF(i);
				ralink_gpio_led_data[i].gpio = -1; //stop
			}
#if RALINK_LED_DEBUG
			printk("T%d\n", ralink_gpio_led_stat[i].times);
		} else {
			printk("T@\n");
#endif
		}
	}
#elif defined (RALINK_GPIO_HAS_5124)
	for (i = 24; i < 40; i++) {
		ralink_gpio_led_stat[i].ticks++;
		if (ralink_gpio_led_data[i].gpio == -1) //-1 means unused
			continue;
		if (ralink_gpio_led_data[i].on == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].off == 0) { //always on
			__LED3924_ON(i);
			continue;
		}
		if (ralink_gpio_led_data[i].off == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].rests == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].on == 0 ||
				ralink_gpio_led_data[i].blinks == 0 ||
				ralink_gpio_led_data[i].times == 0) { //always off
			__LED3924_OFF(i);
			continue;
		}

		//led turn on or off
		if (ralink_gpio_led_data[i].blinks == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].rests == 0) { //always blinking
			x = ralink_gpio_led_stat[i].ticks % (ralink_gpio_led_data[i].on
					+ ralink_gpio_led_data[i].off);
		}
		else {
			unsigned int a, b, c, d, o, t;
			a = ralink_gpio_led_data[i].blinks / 2;
			b = ralink_gpio_led_data[i].rests / 2;
			c = ralink_gpio_led_data[i].blinks % 2;
			d = ralink_gpio_led_data[i].rests % 2;
			o = ralink_gpio_led_data[i].on + ralink_gpio_led_data[i].off;
			//t = blinking ticks
			t = a * o + ralink_gpio_led_data[i].on * c;
			//x = ticks % (blinking ticks + resting ticks)
			x = ralink_gpio_led_stat[i].ticks %
				(t + b * o + ralink_gpio_led_data[i].on * d);
			//starts from 0 at resting cycles
			if (x >= t)
				x -= t;
			x %= o;
		}
		if (x < ralink_gpio_led_data[i].on) {
			__LED3924_ON(i);
			if (ralink_gpio_led_stat[i].ticks && x == 0)
				ralink_gpio_led_stat[i].offs++;
#if RALINK_LED_DEBUG
			printk("t%d gpio%d on,", ralink_gpio_led_stat[i].ticks, i);
#endif
		}
		else {
			__LED3924_OFF(i);
			if (x == ralink_gpio_led_data[i].on)
				ralink_gpio_led_stat[i].ons++;
#if RALINK_LED_DEBUG
			printk("t%d gpio%d off,", ralink_gpio_led_stat[i].ticks, i);
#endif
		}

		//blinking or resting
		if (ralink_gpio_led_data[i].blinks == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].rests == 0) { //always blinking
			continue;
		}
		else {
			x = ralink_gpio_led_stat[i].ons + ralink_gpio_led_stat[i].offs;
			if (!ralink_gpio_led_stat[i].resting) {
				if (x == ralink_gpio_led_data[i].blinks) {
					ralink_gpio_led_stat[i].resting = 1;
					ralink_gpio_led_stat[i].ons = 0;
					ralink_gpio_led_stat[i].offs = 0;
					ralink_gpio_led_stat[i].times++;
				}
			}
			else {
				if (x == ralink_gpio_led_data[i].rests) {
					ralink_gpio_led_stat[i].resting = 0;
					ralink_gpio_led_stat[i].ons = 0;
					ralink_gpio_led_stat[i].offs = 0;
				}
			}
		}
		if (ralink_gpio_led_stat[i].resting) {
			__LED3924_OFF(i);
#if RALINK_LED_DEBUG
			printk("resting,");
		} else {
			printk("blinking,");
#endif
		}

		//number of times
		if (ralink_gpio_led_data[i].times != RALINK_GPIO_LED_INFINITY)
		{
			if (ralink_gpio_led_stat[i].times ==
					ralink_gpio_led_data[i].times) {
				__LED3924_OFF(i);
				ralink_gpio_led_data[i].gpio = -1; //stop
			}
#if RALINK_LED_DEBUG
			printk("T%d\n", ralink_gpio_led_stat[i].times);
		} else {
			printk("T@\n");
#endif
		}
	}

	for (i = 40; i < RALINK_GPIO_NUMBER; i++) {
		ralink_gpio_led_stat[i].ticks++;
		if (ralink_gpio_led_data[i].gpio == -1) //-1 means unused
			continue;
		if (ralink_gpio_led_data[i].on == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].off == 0) { //always on
			__LED5140_ON(i);
			continue;
		}
		if (ralink_gpio_led_data[i].off == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].rests == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].on == 0 ||
				ralink_gpio_led_data[i].blinks == 0 ||
				ralink_gpio_led_data[i].times == 0) { //always off
			__LED5140_OFF(i);
			continue;
		}

		//led turn on or off
		if (ralink_gpio_led_data[i].blinks == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].rests == 0) { //always blinking
			x = ralink_gpio_led_stat[i].ticks % (ralink_gpio_led_data[i].on
					+ ralink_gpio_led_data[i].off);
		}
		else {
			unsigned int a, b, c, d, o, t;
			a = ralink_gpio_led_data[i].blinks / 2;
			b = ralink_gpio_led_data[i].rests / 2;
			c = ralink_gpio_led_data[i].blinks % 2;
			d = ralink_gpio_led_data[i].rests % 2;
			o = ralink_gpio_led_data[i].on + ralink_gpio_led_data[i].off;
			//t = blinking ticks
			t = a * o + ralink_gpio_led_data[i].on * c;
			//x = ticks % (blinking ticks + resting ticks)
			x = ralink_gpio_led_stat[i].ticks %
				(t + b * o + ralink_gpio_led_data[i].on * d);
			//starts from 0 at resting cycles
			if (x >= t)
				x -= t;
			x %= o;
		}
		if (x < ralink_gpio_led_data[i].on) {
			__LED5140_ON(i);
			if (ralink_gpio_led_stat[i].ticks && x == 0)
				ralink_gpio_led_stat[i].offs++;
#if RALINK_LED_DEBUG
			printk("t%d gpio%d on,", ralink_gpio_led_stat[i].ticks, i);
#endif
		}
		else {
			__LED5140_OFF(i);
			if (x == ralink_gpio_led_data[i].on)
				ralink_gpio_led_stat[i].ons++;
#if RALINK_LED_DEBUG
			printk("t%d gpio%d off,", ralink_gpio_led_stat[i].ticks, i);
#endif
		}

		//blinking or resting
		if (ralink_gpio_led_data[i].blinks == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].rests == 0) { //always blinking
			continue;
		}
		else {
			x = ralink_gpio_led_stat[i].ons + ralink_gpio_led_stat[i].offs;
			if (!ralink_gpio_led_stat[i].resting) {
				if (x == ralink_gpio_led_data[i].blinks) {
					ralink_gpio_led_stat[i].resting = 1;
					ralink_gpio_led_stat[i].ons = 0;
					ralink_gpio_led_stat[i].offs = 0;
					ralink_gpio_led_stat[i].times++;
				}
			}
			else {
				if (x == ralink_gpio_led_data[i].rests) {
					ralink_gpio_led_stat[i].resting = 0;
					ralink_gpio_led_stat[i].ons = 0;
					ralink_gpio_led_stat[i].offs = 0;
				}
			}
		}
		if (ralink_gpio_led_stat[i].resting) {
			__LED5140_OFF(i);
#if RALINK_LED_DEBUG
			printk("resting,");
		} else {
			printk("blinking,");
#endif
		}

		//number of times
		if (ralink_gpio_led_data[i].times != RALINK_GPIO_LED_INFINITY)
		{
			if (ralink_gpio_led_stat[i].times ==
					ralink_gpio_led_data[i].times) {
				__LED5140_OFF(i);
				ralink_gpio_led_data[i].gpio = -1; //stop
			}
#if RALINK_LED_DEBUG
			printk("T%d\n", ralink_gpio_led_stat[i].times);
		} else {
			printk("T@\n");
#endif
		}
	}
#elif defined (RALINK_GPIO_HAS_9532)
	for (i = 32; i < 64; i++) {
		ralink_gpio_led_stat[i].ticks++;
		if (ralink_gpio_led_data[i].gpio == -1) //-1 means unused
			continue;
		if (ralink_gpio_led_data[i].on == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].off == 0) { //always on
			__LED6332_ON(i);
			continue;
		}
		if (ralink_gpio_led_data[i].off == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].rests == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].on == 0 ||
				ralink_gpio_led_data[i].blinks == 0 ||
				ralink_gpio_led_data[i].times == 0) { //always off
			__LED6332_OFF(i);
			continue;
		}

		//led turn on or off
		if (ralink_gpio_led_data[i].blinks == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].rests == 0) { //always blinking
			x = ralink_gpio_led_stat[i].ticks % (ralink_gpio_led_data[i].on
					+ ralink_gpio_led_data[i].off);
		}
		else {
			unsigned int a, b, c, d, o, t;
			a = ralink_gpio_led_data[i].blinks / 2;
			b = ralink_gpio_led_data[i].rests / 2;
			c = ralink_gpio_led_data[i].blinks % 2;
			d = ralink_gpio_led_data[i].rests % 2;
			o = ralink_gpio_led_data[i].on + ralink_gpio_led_data[i].off;
			//t = blinking ticks
			t = a * o + ralink_gpio_led_data[i].on * c;
			//x = ticks % (blinking ticks + resting ticks)
			x = ralink_gpio_led_stat[i].ticks %
				(t + b * o + ralink_gpio_led_data[i].on * d);
			//starts from 0 at resting cycles
			if (x >= t)
				x -= t;
			x %= o;
		}
		if (x < ralink_gpio_led_data[i].on) {
			__LED6332_ON(i);
			if (ralink_gpio_led_stat[i].ticks && x == 0)
				ralink_gpio_led_stat[i].offs++;
#if RALINK_LED_DEBUG
			printk("t%d gpio%d on,", ralink_gpio_led_stat[i].ticks, i);
#endif
		}
		else {
			__LED6332_OFF(i);
			if (x == ralink_gpio_led_data[i].on)
				ralink_gpio_led_stat[i].ons++;
#if RALINK_LED_DEBUG
			printk("t%d gpio%d off,", ralink_gpio_led_stat[i].ticks, i);
#endif
		}

		//blinking or resting
		if (ralink_gpio_led_data[i].blinks == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].rests == 0) { //always blinking
			continue;
		}
		else {
			x = ralink_gpio_led_stat[i].ons + ralink_gpio_led_stat[i].offs;
			if (!ralink_gpio_led_stat[i].resting) {
				if (x == ralink_gpio_led_data[i].blinks) {
					ralink_gpio_led_stat[i].resting = 1;
					ralink_gpio_led_stat[i].ons = 0;
					ralink_gpio_led_stat[i].offs = 0;
					ralink_gpio_led_stat[i].times++;
				}
			}
			else {
				if (x == ralink_gpio_led_data[i].rests) {
					ralink_gpio_led_stat[i].resting = 0;
					ralink_gpio_led_stat[i].ons = 0;
					ralink_gpio_led_stat[i].offs = 0;
				}
			}
		}
		if (ralink_gpio_led_stat[i].resting) {
			__LED6332_OFF(i);
#if RALINK_LED_DEBUG
			printk("resting,");
		} else {
			printk("blinking,");
#endif
		}

		//number of times
		if (ralink_gpio_led_data[i].times != RALINK_GPIO_LED_INFINITY)
		{
			if (ralink_gpio_led_stat[i].times ==
					ralink_gpio_led_data[i].times) {
				__LED6332_OFF(i);
				ralink_gpio_led_data[i].gpio = -1; //stop
			}
#if RALINK_LED_DEBUG
			printk("T%d\n", ralink_gpio_led_stat[i].times);
		} else {
			printk("T@\n");
#endif
		}
	}

	for (i = 64; i < RALINK_GPIO_NUMBER; i++) {
		ralink_gpio_led_stat[i].ticks++;
		if (ralink_gpio_led_data[i].gpio == -1) //-1 means unused
			continue;
		if (ralink_gpio_led_data[i].on == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].off == 0) { //always on
			__LED9564_ON(i);
			continue;
		}
		if (ralink_gpio_led_data[i].off == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].rests == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].on == 0 ||
				ralink_gpio_led_data[i].blinks == 0 ||
				ralink_gpio_led_data[i].times == 0) { //always off
			__LED9564_OFF(i);
			continue;
		}

		//led turn on or off
		if (ralink_gpio_led_data[i].blinks == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].rests == 0) { //always blinking
			x = ralink_gpio_led_stat[i].ticks % (ralink_gpio_led_data[i].on
					+ ralink_gpio_led_data[i].off);
		}
		else {
			unsigned int a, b, c, d, o, t;
			a = ralink_gpio_led_data[i].blinks / 2;
			b = ralink_gpio_led_data[i].rests / 2;
			c = ralink_gpio_led_data[i].blinks % 2;
			d = ralink_gpio_led_data[i].rests % 2;
			o = ralink_gpio_led_data[i].on + ralink_gpio_led_data[i].off;
			//t = blinking ticks
			t = a * o + ralink_gpio_led_data[i].on * c;
			//x = ticks % (blinking ticks + resting ticks)
			x = ralink_gpio_led_stat[i].ticks %
				(t + b * o + ralink_gpio_led_data[i].on * d);
			//starts from 0 at resting cycles
			if (x >= t)
				x -= t;
			x %= o;
		}
		if (x < ralink_gpio_led_data[i].on) {
			__LED9564_ON(i);
			if (ralink_gpio_led_stat[i].ticks && x == 0)
				ralink_gpio_led_stat[i].offs++;
#if RALINK_LED_DEBUG
			printk("t%d gpio%d on,", ralink_gpio_led_stat[i].ticks, i);
#endif
		}
		else {
			__LED9564_OFF(i);
			if (x == ralink_gpio_led_data[i].on)
				ralink_gpio_led_stat[i].ons++;
#if RALINK_LED_DEBUG
			printk("t%d gpio%d off,", ralink_gpio_led_stat[i].ticks, i);
#endif
		}

		//blinking or resting
		if (ralink_gpio_led_data[i].blinks == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].rests == 0) { //always blinking
			continue;
		}
		else {
			x = ralink_gpio_led_stat[i].ons + ralink_gpio_led_stat[i].offs;
			if (!ralink_gpio_led_stat[i].resting) {
				if (x == ralink_gpio_led_data[i].blinks) {
					ralink_gpio_led_stat[i].resting = 1;
					ralink_gpio_led_stat[i].ons = 0;
					ralink_gpio_led_stat[i].offs = 0;
					ralink_gpio_led_stat[i].times++;
				}
			}
			else {
				if (x == ralink_gpio_led_data[i].rests) {
					ralink_gpio_led_stat[i].resting = 0;
					ralink_gpio_led_stat[i].ons = 0;
					ralink_gpio_led_stat[i].offs = 0;
				}
			}
		}
		if (ralink_gpio_led_stat[i].resting) {
			__LED9564_OFF(i);
#if RALINK_LED_DEBUG
			printk("resting,");
		} else {
			printk("blinking,");
#endif
		}

		//number of times
		if (ralink_gpio_led_data[i].times != RALINK_GPIO_LED_INFINITY)
		{
			if (ralink_gpio_led_stat[i].times ==
					ralink_gpio_led_data[i].times) {
				__LED9564_OFF(i);
				ralink_gpio_led_data[i].gpio = -1; //stop
			}
#if RALINK_LED_DEBUG
			printk("T%d\n", ralink_gpio_led_stat[i].times);
		} else {
			printk("T@\n");
#endif
		}
	}
#elif defined (RALINK_GPIO_HAS_9524) || defined (RALINK_GPIO_HAS_7224)
	for (i = 24; i < 40; i++) {
		ralink_gpio_led_stat[i].ticks++;
		if (ralink_gpio_led_data[i].gpio == -1) //-1 means unused
			continue;
		if (ralink_gpio_led_data[i].on == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].off == 0) { //always on
			__LED3924_ON(i);
			continue;
		}
		if (ralink_gpio_led_data[i].off == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].rests == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].on == 0 ||
				ralink_gpio_led_data[i].blinks == 0 ||
				ralink_gpio_led_data[i].times == 0) { //always off
			__LED3924_OFF(i);
			continue;
		}

		//led turn on or off
		if (ralink_gpio_led_data[i].blinks == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].rests == 0) { //always blinking
			x = ralink_gpio_led_stat[i].ticks % (ralink_gpio_led_data[i].on
					+ ralink_gpio_led_data[i].off);
		}
		else {
			unsigned int a, b, c, d, o, t;
			a = ralink_gpio_led_data[i].blinks / 2;
			b = ralink_gpio_led_data[i].rests / 2;
			c = ralink_gpio_led_data[i].blinks % 2;
			d = ralink_gpio_led_data[i].rests % 2;
			o = ralink_gpio_led_data[i].on + ralink_gpio_led_data[i].off;
			//t = blinking ticks
			t = a * o + ralink_gpio_led_data[i].on * c;
			//x = ticks % (blinking ticks + resting ticks)
			x = ralink_gpio_led_stat[i].ticks %
				(t + b * o + ralink_gpio_led_data[i].on * d);
			//starts from 0 at resting cycles
			if (x >= t)
				x -= t;
			x %= o;
		}
		if (x < ralink_gpio_led_data[i].on) {
			__LED3924_ON(i);
			if (ralink_gpio_led_stat[i].ticks && x == 0)
				ralink_gpio_led_stat[i].offs++;
#if RALINK_LED_DEBUG
			printk("t%d gpio%d on,", ralink_gpio_led_stat[i].ticks, i);
#endif
		}
		else {
			__LED3924_OFF(i);
			if (x == ralink_gpio_led_data[i].on)
				ralink_gpio_led_stat[i].ons++;
#if RALINK_LED_DEBUG
			printk("t%d gpio%d off,", ralink_gpio_led_stat[i].ticks, i);
#endif
		}

		//blinking or resting
		if (ralink_gpio_led_data[i].blinks == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].rests == 0) { //always blinking
			continue;
		}
		else {
			x = ralink_gpio_led_stat[i].ons + ralink_gpio_led_stat[i].offs;
			if (!ralink_gpio_led_stat[i].resting) {
				if (x == ralink_gpio_led_data[i].blinks) {
					ralink_gpio_led_stat[i].resting = 1;
					ralink_gpio_led_stat[i].ons = 0;
					ralink_gpio_led_stat[i].offs = 0;
					ralink_gpio_led_stat[i].times++;
				}
			}
			else {
				if (x == ralink_gpio_led_data[i].rests) {
					ralink_gpio_led_stat[i].resting = 0;
					ralink_gpio_led_stat[i].ons = 0;
					ralink_gpio_led_stat[i].offs = 0;
				}
			}
		}
		if (ralink_gpio_led_stat[i].resting) {
			__LED3924_OFF(i);
#if RALINK_LED_DEBUG
			printk("resting,");
		} else {
			printk("blinking,");
#endif
		}

		//number of times
		if (ralink_gpio_led_data[i].times != RALINK_GPIO_LED_INFINITY)
		{
			if (ralink_gpio_led_stat[i].times ==
					ralink_gpio_led_data[i].times) {
				__LED3924_OFF(i);
				ralink_gpio_led_data[i].gpio = -1; //stop
			}
#if RALINK_LED_DEBUG
			printk("T%d\n", ralink_gpio_led_stat[i].times);
		} else {
			printk("T@\n");
#endif
		}
	}

	for (i = 40; i < 72; i++) {
		ralink_gpio_led_stat[i].ticks++;
		if (ralink_gpio_led_data[i].gpio == -1) //-1 means unused
			continue;
		if (ralink_gpio_led_data[i].on == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].off == 0) { //always on
			__LED7140_ON(i);
			continue;
		}
		if (ralink_gpio_led_data[i].off == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].rests == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].on == 0 ||
				ralink_gpio_led_data[i].blinks == 0 ||
				ralink_gpio_led_data[i].times == 0) { //always off
			__LED7140_OFF(i);
			continue;
		}

		//led turn on or off
		if (ralink_gpio_led_data[i].blinks == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].rests == 0) { //always blinking
			x = ralink_gpio_led_stat[i].ticks % (ralink_gpio_led_data[i].on
					+ ralink_gpio_led_data[i].off);
		}
		else {
			unsigned int a, b, c, d, o, t;
			a = ralink_gpio_led_data[i].blinks / 2;
			b = ralink_gpio_led_data[i].rests / 2;
			c = ralink_gpio_led_data[i].blinks % 2;
			d = ralink_gpio_led_data[i].rests % 2;
			o = ralink_gpio_led_data[i].on + ralink_gpio_led_data[i].off;
			//t = blinking ticks
			t = a * o + ralink_gpio_led_data[i].on * c;
			//x = ticks % (blinking ticks + resting ticks)
			x = ralink_gpio_led_stat[i].ticks %
				(t + b * o + ralink_gpio_led_data[i].on * d);
			//starts from 0 at resting cycles
			if (x >= t)
				x -= t;
			x %= o;
		}
		if (x < ralink_gpio_led_data[i].on) {
			__LED7140_ON(i);
			if (ralink_gpio_led_stat[i].ticks && x == 0)
				ralink_gpio_led_stat[i].offs++;
#if RALINK_LED_DEBUG
			printk("t%d gpio%d on,", ralink_gpio_led_stat[i].ticks, i);
#endif
		}
		else {
			__LED7140_OFF(i);
			if (x == ralink_gpio_led_data[i].on)
				ralink_gpio_led_stat[i].ons++;
#if RALINK_LED_DEBUG
			printk("t%d gpio%d off,", ralink_gpio_led_stat[i].ticks, i);
#endif
		}

		//blinking or resting
		if (ralink_gpio_led_data[i].blinks == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].rests == 0) { //always blinking
			continue;
		}
		else {
			x = ralink_gpio_led_stat[i].ons + ralink_gpio_led_stat[i].offs;
			if (!ralink_gpio_led_stat[i].resting) {
				if (x == ralink_gpio_led_data[i].blinks) {
					ralink_gpio_led_stat[i].resting = 1;
					ralink_gpio_led_stat[i].ons = 0;
					ralink_gpio_led_stat[i].offs = 0;
					ralink_gpio_led_stat[i].times++;
				}
			}
			else {
				if (x == ralink_gpio_led_data[i].rests) {
					ralink_gpio_led_stat[i].resting = 0;
					ralink_gpio_led_stat[i].ons = 0;
					ralink_gpio_led_stat[i].offs = 0;
				}
			}
		}
		if (ralink_gpio_led_stat[i].resting) {
			__LED7140_OFF(i);
#if RALINK_LED_DEBUG
			printk("resting,");
		} else {
			printk("blinking,");
#endif
		}

		//number of times
		if (ralink_gpio_led_data[i].times != RALINK_GPIO_LED_INFINITY)
		{
			if (ralink_gpio_led_stat[i].times ==
					ralink_gpio_led_data[i].times) {
				__LED7140_OFF(i);
				ralink_gpio_led_data[i].gpio = -1; //stop
			}
#if RALINK_LED_DEBUG
			printk("T%d\n", ralink_gpio_led_stat[i].times);
		} else {
			printk("T@\n");
#endif
		}
	}

	for (i = 72; i < RALINK_GPIO_NUMBER; i++) {
		ralink_gpio_led_stat[i].ticks++;
		if (ralink_gpio_led_data[i].gpio == -1) //-1 means unused
			continue;
		if (ralink_gpio_led_data[i].on == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].off == 0) { //always on
#if defined (RALINK_GPIO_HAS_7224)
			__LED72_ON(i);
#else
			__LED9572_ON(i);
#endif
			continue;
		}
		if (ralink_gpio_led_data[i].off == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].rests == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].on == 0 ||
				ralink_gpio_led_data[i].blinks == 0 ||
				ralink_gpio_led_data[i].times == 0) { //always off
#if defined (RALINK_GPIO_HAS_7224)
			__LED72_OFF(i);
#else
			__LED9572_OFF(i);
#endif
			continue;
		}

		//led turn on or off
		if (ralink_gpio_led_data[i].blinks == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].rests == 0) { //always blinking
			x = ralink_gpio_led_stat[i].ticks % (ralink_gpio_led_data[i].on
					+ ralink_gpio_led_data[i].off);
		}
		else {
			unsigned int a, b, c, d, o, t;
			a = ralink_gpio_led_data[i].blinks / 2;
			b = ralink_gpio_led_data[i].rests / 2;
			c = ralink_gpio_led_data[i].blinks % 2;
			d = ralink_gpio_led_data[i].rests % 2;
			o = ralink_gpio_led_data[i].on + ralink_gpio_led_data[i].off;
			//t = blinking ticks
			t = a * o + ralink_gpio_led_data[i].on * c;
			//x = ticks % (blinking ticks + resting ticks)
			x = ralink_gpio_led_stat[i].ticks %
				(t + b * o + ralink_gpio_led_data[i].on * d);
			//starts from 0 at resting cycles
			if (x >= t)
				x -= t;
			x %= o;
		}
		if (x < ralink_gpio_led_data[i].on) {
#if defined (RALINK_GPIO_HAS_7224)
			__LED72_ON(i);
#else
			__LED9572_ON(i);
#endif
			if (ralink_gpio_led_stat[i].ticks && x == 0)
				ralink_gpio_led_stat[i].offs++;
#if RALINK_LED_DEBUG
			printk("t%d gpio%d on,", ralink_gpio_led_stat[i].ticks, i);
#endif
		}
		else {
#if defined (RALINK_GPIO_HAS_7224)
			__LED72_OFF(i);
#else
			__LED9572_OFF(i);
#endif
			if (x == ralink_gpio_led_data[i].on)
				ralink_gpio_led_stat[i].ons++;
#if RALINK_LED_DEBUG
			printk("t%d gpio%d off,", ralink_gpio_led_stat[i].ticks, i);
#endif
		}

		//blinking or resting
		if (ralink_gpio_led_data[i].blinks == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].rests == 0) { //always blinking
			continue;
		}
		else {
			x = ralink_gpio_led_stat[i].ons + ralink_gpio_led_stat[i].offs;
			if (!ralink_gpio_led_stat[i].resting) {
				if (x == ralink_gpio_led_data[i].blinks) {
					ralink_gpio_led_stat[i].resting = 1;
					ralink_gpio_led_stat[i].ons = 0;
					ralink_gpio_led_stat[i].offs = 0;
					ralink_gpio_led_stat[i].times++;
				}
			}
			else {
				if (x == ralink_gpio_led_data[i].rests) {
					ralink_gpio_led_stat[i].resting = 0;
					ralink_gpio_led_stat[i].ons = 0;
					ralink_gpio_led_stat[i].offs = 0;
				}
			}
		}
		if (ralink_gpio_led_stat[i].resting) {
#if defined (RALINK_GPIO_HAS_7224)
			__LED72_OFF(i);
#else
			__LED9572_OFF(i);
#endif
#if RALINK_LED_DEBUG
			printk("resting,");
		} else {
			printk("blinking,");
#endif
		}

		//number of times
		if (ralink_gpio_led_data[i].times != RALINK_GPIO_LED_INFINITY)
		{
			if (ralink_gpio_led_stat[i].times ==
					ralink_gpio_led_data[i].times) {
#if defined (RALINK_GPIO_HAS_7224)
				__LED72_OFF(i);
#else
				__LED9572_OFF(i);
#endif
				ralink_gpio_led_data[i].gpio = -1; //stop
			}
#if RALINK_LED_DEBUG
			printk("T%d\n", ralink_gpio_led_stat[i].times);
		} else {
			printk("T@\n");
#endif
		}
	}
#endif
#endif

	//always turn the power LED on
#ifdef CONFIG_RALINK_RT2880
	__LED_ON(12);
#elif defined (CONFIG_RALINK_RT3052) || defined (CONFIG_RALINK_RT2883)
	__LED_ON(9);
#endif

	*(volatile u32 *)(RALINK_REG_PIORESET) = ra_gpio_led_clr;
	*(volatile u32 *)(RALINK_REG_PIOSET) = ra_gpio_led_set;
#if defined (RALINK_GPIO_HAS_2722)
	*(volatile u32 *)(RALINK_REG_PIO2722RESET) = ra_gpio2722_led_clr;
	*(volatile u32 *)(RALINK_REG_PIO2722SET) = ra_gpio2722_led_set;
#elif defined (RALINK_GPIO_HAS_4524)
	*(volatile u32 *)(RALINK_REG_PIO3924RESET) = ra_gpio3924_led_clr;
	*(volatile u32 *)(RALINK_REG_PIO3924SET) = ra_gpio3924_led_set;
	*(volatile u32 *)(RALINK_REG_PIO4540RESET) = ra_gpio4540_led_clr;
	*(volatile u32 *)(RALINK_REG_PIO4540SET) = ra_gpio4540_led_set;
#elif defined (RALINK_GPIO_HAS_5124)
	*(volatile u32 *)(RALINK_REG_PIO3924RESET) = ra_gpio3924_led_clr;
	*(volatile u32 *)(RALINK_REG_PIO3924SET) = ra_gpio3924_led_set;
	*(volatile u32 *)(RALINK_REG_PIO5140RESET) = ra_gpio5140_led_clr;
	*(volatile u32 *)(RALINK_REG_PIO5140SET) = ra_gpio5140_led_set;
#elif defined (RALINK_GPIO_HAS_9524) || defined (RALINK_GPIO_HAS_7224)
	*(volatile u32 *)(RALINK_REG_PIO3924RESET) = ra_gpio3924_led_clr;
	*(volatile u32 *)(RALINK_REG_PIO3924SET) = ra_gpio3924_led_set;
	*(volatile u32 *)(RALINK_REG_PIO7140RESET) = ra_gpio7140_led_clr;
	*(volatile u32 *)(RALINK_REG_PIO7140SET) = ra_gpio7140_led_set;
#if defined (RALINK_GPIO_HAS_7224)
	*(volatile u32 *)(RALINK_REG_PIO72RESET) = ra_gpio72_led_clr;
	*(volatile u32 *)(RALINK_REG_PIO72SET) = ra_gpio72_led_set;
#else
	*(volatile u32 *)(RALINK_REG_PIO9572RESET) = ra_gpio9572_led_clr;
	*(volatile u32 *)(RALINK_REG_PIO9572SET) = ra_gpio9572_led_set;
#endif
#elif defined (RALINK_GPIO_HAS_9532)
	*(volatile u32 *)(RALINK_REG_PIO6332RESET) = ra_gpio6332_led_clr;
	*(volatile u32 *)(RALINK_REG_PIO6332SET) = ra_gpio6332_led_set;
	*(volatile u32 *)(RALINK_REG_PIO9564RESET) = ra_gpio9564_led_clr;
	*(volatile u32 *)(RALINK_REG_PIO9564SET) = ra_gpio9564_led_set;
#endif

#if RALINK_LED_DEBUG
	printk("led_set= %x, led_clr= %x\n", ra_gpio_led_set, ra_gpio_led_clr);
#if defined (RALINK_GPIO_HAS_2722)
	printk("led2722_set= %x, led2722_clr= %x\n", ra_gpio2722_led_set, ra_gpio2722_led_clr);
#elif defined (RALINK_GPIO_HAS_4524)
	printk("led3924_set= %x, led3924_clr= %x\n", ra_gpio3924_led_set, ra_gpio3924_led_clr);
	printk("led4540_set= %x, led4540_clr= %x\n", ra_gpio4540_led_set, ra_gpio4540_led_set);
#elif defined (RALINK_GPIO_HAS_5124)
	printk("led3924_set= %x, led3924_clr= %x\n", ra_gpio3924_led_set, ra_gpio3924_led_clr);
	printk("led5140_set= %x, led5140_clr= %x\n", ra_gpio5140_led_set, ra_gpio5140_led_set);
#elif defined (RALINK_GPIO_HAS_9524) || defined (RALINK_GPIO_HAS_7224)
	printk("led3924_set= %x, led3924_clr= %x\n", ra_gpio3924_led_set, ra_gpio3924_led_clr);
	printk("led7140_set= %x, led7140_clr= %x\n", ra_gpio7140_led_set, ra_gpio7140_led_set);
#if defined (RALINK_GPIO_HAS_7224)
	printk("led72_set= %x, led72_clr= %x\n", ra_gpio72_led_set, ra_gpio72_led_set);
#else
	printk("led9572_set= %x, led9572_clr= %x\n", ra_gpio9572_led_set, ra_gpio9572_led_set);
#endif
#elif defined (RALINK_GPIO_HAS_9532)
	printk("led6332_set= %x, led6332_clr= %x\n", ra_gpio6332_led_set, ra_gpio6332_led_clr);
	printk("led9564_set= %x, led9564_clr= %x\n", ra_gpio9564_led_set, ra_gpio9564_led_set);
#endif
#endif

	ra_gpio_led_set = ra_gpio_led_clr = 0;
#if defined (RALINK_GPIO_HAS_2722)
	ra_gpio2722_led_set = ra_gpio2722_led_clr = 0;
#elif defined (RALINK_GPIO_HAS_4524)
	ra_gpio3924_led_set = ra_gpio3924_led_clr = 0;
	ra_gpio4540_led_set = ra_gpio4540_led_clr = 0;
#elif defined (RALINK_GPIO_HAS_5124)
	ra_gpio3924_led_set = ra_gpio3924_led_clr = 0;
	ra_gpio5140_led_set = ra_gpio5140_led_clr = 0;
#elif defined (RALINK_GPIO_HAS_9524) || defined (RALINK_GPIO_HAS_7224)
	ra_gpio3924_led_set = ra_gpio3924_led_clr = 0;
	ra_gpio7140_led_set = ra_gpio7140_led_clr = 0;
#if defined (RALINK_GPIO_HAS_7224)
	ra_gpio72_led_set = ra_gpio72_led_clr = 0;
#else
	ra_gpio9572_led_set = ra_gpio9572_led_clr = 0;
#endif
#elif defined (RALINK_GPIO_HAS_9532)
	ra_gpio6332_led_set = ra_gpio6332_led_clr = 0;
	ra_gpio9564_led_set = ra_gpio9564_led_clr = 0;
#endif

#ifdef CONFIG_EFM_PATCH
        check_default_reset();
#endif

	init_timer(&ralink_gpio_led_timer);
	ralink_gpio_led_timer.expires = jiffies + RALINK_GPIO_LED_FREQ;
	add_timer(&ralink_gpio_led_timer);

}

void ralink_gpio_led_init_timer(void)
{
	int i;

	for (i = 0; i < RALINK_GPIO_NUMBER; i++)
		ralink_gpio_led_data[i].gpio = -1; //-1 means unused
#ifdef CONFIG_EFM_PATCH
        //init_iptime_gpio_standard();
#else

#if RALINK_GPIO_LED_LOW_ACT
	ra_gpio_led_set = 0xffffffff;
#if defined (RALINK_GPIO_HAS_2722)
	ra_gpio2722_led_set = 0xff;
#elif defined (RALINK_GPIO_HAS_4524)
	ra_gpio3924_led_set = 0xffff;
	ra_gpio4540_led_set = 0xff;
#elif defined (RALINK_GPIO_HAS_5124)
	ra_gpio3924_led_set = 0xffff;
	ra_gpio5140_led_set = 0xfff;
#elif defined (RALINK_GPIO_HAS_9524) || defined (RALINK_GPIO_HAS_7224)
	ra_gpio3924_led_set = 0xffff;
	ra_gpio7140_led_set = 0xffffffff;
#if defined (RALINK_GPIO_HAS_7224)
	ra_gpio72_led_set = 0xffffff;
#else
	ra_gpio9572_led_set = 0xffffff;
#endif
#elif defined (RALINK_GPIO_HAS_9532)
	ra_gpio6332_led_set = 0xffffffff;
	ra_gpio9564_led_set = 0xffffffff;
#endif
#else // RALINK_GPIO_LED_LOW_ACT //
	ra_gpio_led_clr = 0xffffffff;
#if defined (RALINK_GPIO_HAS_2722)
	ra_gpio2722_led_clr = 0xff;
#elif defined (RALINK_GPIO_HAS_4524)
	ra_gpio3924_led_clr = 0xffff;
	ra_gpio4540_led_clr = 0xff;
#elif defined (RALINK_GPIO_HAS_5124)
	ra_gpio3924_led_clr = 0xffff;
	ra_gpio5140_led_clr = 0xfff;
#elif defined (RALINK_GPIO_HAS_9524) || defined (RALINK_GPIO_HAS_7224)
	ra_gpio3924_led_clr = 0xffff;
	ra_gpio7140_led_clr = 0xffffffff;
#if defined (RALINK_GPIO_HAS_7224)
	ra_gpio72_led_clr = 0xffffff;
#else
	ra_gpio9572_led_clr = 0xffffff;
#endif
#elif defined (RALINK_GPIO_HAS_9532)
	ra_gpio6332_led_clr = 0xffffffff;
	ra_gpio9564_led_clr = 0xffffffff;
#endif
#endif // RALINK_GPIO_LED_LOW_ACT //
#endif

	init_timer(&ralink_gpio_led_timer);
	ralink_gpio_led_timer.function = ralink_gpio_led_do_timer;
	ralink_gpio_led_timer.expires = jiffies + RALINK_GPIO_LED_FREQ;
	add_timer(&ralink_gpio_led_timer);
}
#endif


#ifdef CONFIG_EFM_PATCH
#include <linux/seq_file.h>
#include <linux/proc_fs.h>

static int proc_read_gpiomode(struct seq_file *seq, void *v)
{
        seq_printf(seq, "gpiomode:%08x PIORENA:%08x PIOFENA:%08x DIR:%08x INTENA:%08x\n",
                *(volatile u32 *)(RALINK_REG_GPIOMODE),
                *(volatile u32 *)(RALINK_REG_PIORENA),
                *(volatile u32 *)(RALINK_REG_PIOFENA),
                *(volatile u32 *)(RALINK_REG_PIODIR),
                *(volatile u32 *)(RALINK_REG_INTENA)
                );

        return 0;
}

static int proc_open_gpiomode(struct inode *inode, struct file *file)
{
	return single_open(file, proc_read_gpiomode, NULL);
}


static int proc_write_gpiomode(struct file *file, const char __user * buffer, size_t count, loff_t *ppos)
{
        unsigned int value;
        char strval[16];

        if(count < 8) return count;

        memset(strval,0,16);
        memcpy(strval,buffer,8);
        value=simple_strtol(buffer,0,16);
printk("write value => %08x\n", value );
        *(volatile u32 *)(RALINK_REG_GPIOMODE) = value;

        return count;
}

static int proc_read_reset_bt(struct seq_file *seq, void *v)
{
        if(!check_reset_bt())
                seq_printf(seq, "1");
        else
                seq_printf(seq, "0");
        return 0;
}

static int proc_open_reset_bt(struct inode *inode, struct file *file)
{
	return single_open(file, proc_read_reset_bt, NULL);
}


static int proc_write_reset_bt(struct file *file, const char __user * buffer, size_t count, loff_t *ppos)
{
#if     0
        if(!strncmp(buffer,"start",5))
        {
                reset_op_start=1;
                printk("Reset Button Start\n");
        }
        else
#endif
        if(!strncmp(buffer,"wps_function_off",5))
                use_wps_only_button = 1;

        return count;
}

static int proc_read_gpio_init(struct seq_file *seq, void *v)
{
        seq_printf(seq,"cpu_led:%d reset_bt:%d wps_bt:%d usb_led:%d\n",runled_gpio_num,reset_gpio_num,wps_gpio_num,usbled_gpio_num);

        return 0;
}

static int proc_open_gpio_init(struct inode *inode, struct file *file)
{
	return single_open(file, proc_read_gpio_init, NULL);
}

static int proc_write_gpio_init(struct file *file, const char __user * buffer, size_t count, loff_t *ppos)
{
        char buf[128];
        int len;
        char *start,*ptr;

        if(count>=128) len=127;
        else len=count;
        memcpy(buf,buffer,len);
        buf[len]=0;
        printk("set string: %s\n", buf);

        start=buf;

        ptr=strchr(start,',');
        if(!ptr) return count;
        *ptr=0; runled_gpio_num=atoi(start); start=ptr+1;

        ptr=strchr(start,',');
        if(!ptr) return count;
        *ptr=0; reset_gpio_num=atoi(start); start=ptr+1;

        ptr=strchr(start,',');
        if(ptr && (start[0]!='-'))
	{
		*ptr=0;
        	wps_gpio_num=atoi(start);
	}

	if(ptr)
	{
		start=ptr+1;
        	ptr=strchr(start,',');
        	if(ptr && (start[0]!='-'))
		{
			*ptr=0;
        		usbled_gpio_num=atoi(start);
		}
	}

        gpio_profile=EFM_GPIO_PROFILE_COMMON;
        init_iptime_gpio_standard();

        printk("cpu_led:%d reset_bt:%d wps_bt:%d usbled:%d\n",runled_gpio_num,reset_gpio_num,wps_gpio_num, usbled_gpio_num);

        return count;
}

static int proc_read_gpio_init2(struct seq_file *seq, void *v)
{
        seq_printf(seq,"apstatus_led_num:%d\n",apstatus_led_gpio_num);
        return 0;
}

static int proc_open_gpio_init2(struct inode *inode, struct file *file)
{
	return single_open(file, proc_read_gpio_init2, NULL);
}

static int proc_write_gpio_init2(struct file *file, const char __user * buffer, size_t count, loff_t *ppos)
{
        char buf[128];
        int len;

        if(count>=128) len=127;
        else len=count;
        memcpy(buf,buffer,len);
        buf[len]=0;
        printk("set string: %s\n", buf);
	if(buf[0]!='-')
	{
		apstatus_led_gpio_num=atoi(buf);
	}
        printk("apstatus_led:%d\n",apstatus_led_gpio_num);

        return count;
}


static int proc_read_gpio_profile(struct seq_file *seq, void *v)
{
        seq_printf(seq, "%d",gpio_profile);
        return 0;
}

static int proc_open_gpio_profile(struct inode *inode, struct file *file)
{
	return single_open(file, proc_read_gpio_profile, NULL);
}

static int proc_write_gpio_profile(struct file *file, const char __user * buffer, size_t count, loff_t *ppos)
{
        if(buffer[0]=='1')
        {
                /* MT7620A + 11AC BOARD profile (A104NS) */
                gpio_profile=EFM_GPIO_PROFILE_1;

#define PROFILE1_USB1_LED_GPIO_NUM              11
#define PROFILE1_RUN_LED_GPIO_NUM               39
#define PROFILE1_RESET_SWITCH_GPIO_NUM          13
#define PROFILE1_WPS_GPIO_NUM                   12
                reset_gpio_num = PROFILE1_RESET_SWITCH_GPIO_NUM;
                wps_gpio_num = PROFILE1_WPS_GPIO_NUM;
                runled_gpio_num = PROFILE1_RUN_LED_GPIO_NUM;
                usbled_gpio_num = PROFILE1_USB1_LED_GPIO_NUM;
        }
        else if(buffer[0]=='2')
        {
                /* MT7620N + 11N BOARD profile ( N604T ) */
                gpio_profile=EFM_GPIO_PROFILE_2;

#define PROFILE2_RUN_LED_GPIO_NUM               2
#define PROFILE2_RESET_SWITCH_GPIO_NUM          1
                reset_gpio_num = PROFILE2_RESET_SWITCH_GPIO_NUM;
                wps_gpio_num = reset_gpio_num;
                runled_gpio_num = PROFILE2_RUN_LED_GPIO_NUM;
                usbled_gpio_num = -1;
        }
        else if(buffer[0]=='3')
        {
                /* MT7620A + 11AC BOARD profile (A1004) */
                gpio_profile=EFM_GPIO_PROFILE_3;

#define PROFILE3_RUN_LED_GPIO_NUM               11
#define PROFILE3_RESET_SWITCH_GPIO_NUM          1
#define PROFILE3_WPS_GPIO_NUM                   2

                reset_gpio_num = PROFILE3_RESET_SWITCH_GPIO_NUM;
                wps_gpio_num = PROFILE3_WPS_GPIO_NUM;
                runled_gpio_num = PROFILE3_RUN_LED_GPIO_NUM;
                usbled_gpio_num = -1;
        }
        else if(buffer[0]=='4')
	{
		   /* MT7621 + 2x2 11AC BOARD profile (A3004NS+) */
                gpio_profile=EFM_GPIO_PROFILE_4;

#define PROFILE4_RUN_LED_GPIO_NUM               18
#define PROFILE4_RESET_SWITCH_GPIO_NUM          4
#define PROFILE4_WPS_GPIO_NUM                   3
#define PROFILE4_USB1_LED_GPIO_NUM              7
                reset_gpio_num = PROFILE4_RESET_SWITCH_GPIO_NUM;
                wps_gpio_num = PROFILE4_WPS_GPIO_NUM;
                runled_gpio_num = PROFILE4_RUN_LED_GPIO_NUM;
                usbled_gpio_num = PROFILE4_USB1_LED_GPIO_NUM;
	}


        init_iptime_gpio_standard();

        return count;
}



static int proc_read_run_period(struct seq_file *seq, void *v)
{
        seq_printf(seq, "%d",ralink_gpio_led_data[runled_gpio_num].on*10);
        return 0;
}

static int proc_open_run_period(struct inode *inode, struct file *file)
{
	return single_open(file, proc_read_run_period, NULL);
}


static int proc_write_run_period(struct file *file, const char __user * buffer, size_t count, loff_t *ppos)
{
        int period;
        char periodstr[32];

        if(count > 31 )
        {
                printk("run_period:too long\n");
                return count;
        }

        if(runled_gpio_num == -1)
        {
                printk("No run_gpio is defined\n");
                return count;
        }

        memcpy(periodstr, buffer,count);
        periodstr[count]=0;
        period=atoi(periodstr);
        printk("Run Led Period: %d\n", period/10);
        blink_led( runled_gpio_num, period/10 );

        return count;
}
#endif


static struct file_operations gpiomode_fops = {
	.owner          = THIS_MODULE,
	.open           = proc_open_gpiomode,
	.read           = seq_read,
	.llseek         = seq_lseek,
	.write          = proc_write_gpiomode,
	.release        = single_release
};

static struct file_operations reset_bt_fops = {
	.owner          = THIS_MODULE,
	.open           = proc_open_reset_bt,
	.read           = seq_read,
	.llseek         = seq_lseek,
	.write          = proc_write_reset_bt,
	.release        = single_release
};

static struct file_operations gpio_profile_fops = {
	.owner          = THIS_MODULE,
	.open           = proc_open_gpio_profile,
	.read           = seq_read,
	.llseek         = seq_lseek,
	.write          = proc_write_gpio_profile,
	.release        = single_release
};

static struct file_operations gpio_init_fops = {
	.owner          = THIS_MODULE,
	.open           = proc_open_gpio_init,
	.read           = seq_read,
	.llseek         = seq_lseek,
	.write          = proc_write_gpio_init,
	.release        = single_release
};

static struct file_operations gpio_init2_fops = {
	.owner          = THIS_MODULE,
	.open           = proc_open_gpio_init2,
	.read           = seq_read,
	.llseek         = seq_lseek,
	.write          = proc_write_gpio_init2,
	.release        = single_release
};

static struct file_operations run_period_fops = {
	.owner          = THIS_MODULE,
	.open           = proc_open_run_period,
	.read           = seq_read,
	.llseek         = seq_lseek,
	.write          = proc_write_run_period,
	.release        = single_release
};

int __init ralink_gpio_init(void)
{
	unsigned int i;
	u32 gpiomode;

#ifdef  CONFIG_DEVFS_FS
	if (devfs_register_chrdev(ralink_gpio_major, RALINK_GPIO_DEVNAME,
				&ralink_gpio_fops)) {
		printk(KERN_ERR NAME ": unable to register character device\n");
		return -EIO;
	}
	devfs_handle = devfs_register(NULL, RALINK_GPIO_DEVNAME,
			DEVFS_FL_DEFAULT, ralink_gpio_major, 0,
			S_IFCHR | S_IRUGO | S_IWUGO, &ralink_gpio_fops, NULL);
#else
	int r = 0;
	r = register_chrdev(ralink_gpio_major, RALINK_GPIO_DEVNAME,
			&ralink_gpio_fops);
	if (r < 0) {
		printk(KERN_ERR NAME ": unable to register character device\n");
		return r;
	}
	if (ralink_gpio_major == 0) {
		ralink_gpio_major = r;
		printk(KERN_DEBUG NAME ": got dynamic major %d\n", r);
	}
#endif

	//config these pins to gpio mode
	gpiomode = le32_to_cpu(*(volatile u32 *)(RALINK_REG_GPIOMODE));
#if !defined (CONFIG_RALINK_RT2880)
	gpiomode &= ~0x1C;  //clear bit[2:4]UARTF_SHARE_MODE
#endif
#if defined (CONFIG_RALINK_MT7620)
	gpiomode &= ~0x2000;  //clear bit[13] WLAN_LED
#endif
	gpiomode |= RALINK_GPIOMODE_DFT;

#ifdef CONFIG_EFM_PATCH

#ifdef CONFIG_GE_RGMII_MT7530_P0_AN
        gpiomode &= ~RALINK_GPIOMODE_MDIO;
#endif

#ifdef CONFIG_RALINK_MT7620
        gpiomode &= ~RALINK_GPIOMODE_UARTF; /* For UARTF -> GPIO  */
        gpiomode |= 0x14; /* 10100 */

        gpiomode &= ~RALINK_GPIOMODE_SPI; /* For Run LED : SPI_WP -> GPIO */
#endif

#ifdef CONFIG_RALINK_MT7621
        /* A3003NS Board Wireless LED : GPIO14 / GPIO17 : JTAG */
        gpiomode |= RALINK_GPIOMODE_JTAG;
#endif

        {
                char name[128];
                struct proc_dir_entry *proc_entry;

                sprintf(name, "driver/gpiomode");
		if(!(proc_entry = proc_create(name, 0, 0, &gpiomode_fops)))
			printk("!! FAIL to create %s PROC !!\n", name);

                sprintf(name, "driver/reset_bt");
		if(!(proc_entry = proc_create(name, 0, 0, &reset_bt_fops)))
			printk("!! FAIL to create %s PROC !!\n", name);

                sprintf(name, "driver/gpio_profile");
		if(!(proc_entry = proc_create(name, 0, 0, &gpio_profile_fops)))
			printk("!! FAIL to create %s PROC !!\n", name);

                sprintf(name, "driver/gpioinit");
		if(!(proc_entry = proc_create(name, 0, 0, &gpio_init_fops)))
			printk("!! FAIL to create %s PROC !!\n", name);

                sprintf(name, "driver/gpioinit2");
		if(!(proc_entry = proc_create(name, 0, 0, &gpio_init2_fops)))
			printk("!! FAIL to create %s PROC !!\n", name);

                sprintf(name, "driver/run_period");
		if(!(proc_entry = proc_create(name, 0, 0, &run_period_fops)))
			printk("!! FAIL to create %s PROC !!\n", name);
        }
#endif

	*(volatile u32 *)(RALINK_REG_GPIOMODE) = cpu_to_le32(gpiomode);

	//enable gpio interrupt
	*(volatile u32 *)(RALINK_REG_INTENA) = cpu_to_le32(RALINK_INTCTL_PIO);
	for (i = 0; i < RALINK_GPIO_NUMBER; i++) {
		ralink_gpio_info[i].irq = i;
		ralink_gpio_info[i].pid = 0;
	}

#ifdef CONFIG_RALINK_GPIO_LED
	ralink_gpio_led_init_timer();
#endif
	printk("Ralink gpio driver initialized\n");
	return 0;
}

void __exit ralink_gpio_exit(void)
{
#ifdef  CONFIG_DEVFS_FS
	devfs_unregister_chrdev(ralink_gpio_major, RALINK_GPIO_DEVNAME);
	devfs_unregister(devfs_handle);
#else
	unregister_chrdev(ralink_gpio_major, RALINK_GPIO_DEVNAME);
#endif

	//config these pins to normal mode
	*(volatile u32 *)(RALINK_REG_GPIOMODE) &= ~RALINK_GPIOMODE_DFT;
	//disable gpio interrupt
	*(volatile u32 *)(RALINK_REG_INTDIS) = cpu_to_le32(RALINK_INTCTL_PIO);
#ifdef CONFIG_RALINK_GPIO_LED
	del_timer(&ralink_gpio_led_timer);
#endif
	printk("Ralink gpio driver exited\n");
}

/*
 * send a signal(SIGUSR1) to the registered user process whenever any gpio
 * interrupt comes
 * (called by interrupt handler)
 */
void ralink_gpio_notify_user(int usr)
{
	struct task_struct *p = NULL;

	if (ralink_gpio_irqnum < 0 || RALINK_GPIO_NUMBER <= ralink_gpio_irqnum) {
		printk(KERN_ERR NAME ": gpio irq number out of range\n");
		return;
	}

#ifndef CONFIG_EFM_PATCH
	//don't send any signal if pid is 0 or 1
	if ((int)ralink_gpio_info[ralink_gpio_irqnum].pid < 2)
		return;
#else
       /* 1 is allowed */
        if ((int)ralink_gpio_info[ralink_gpio_irqnum].pid == 0)
                return;
#endif
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,35)
	p = find_task_by_vpid(ralink_gpio_info[ralink_gpio_irqnum].pid);
#else
	p = find_task_by_pid(ralink_gpio_info[ralink_gpio_irqnum].pid);
#endif

	if (NULL == p) {
		printk(KERN_ERR NAME ": no registered process to notify\n");
		return;
	}

#ifdef CONFIG_EFM_PATCH
        if (usr == reset_gpio_num)
        {
                if(reset_pushed)
                {
                        printk(KERN_NOTICE NAME ": sending a SIGUSR2 to process %d\n",
                                        ralink_gpio_info[ralink_gpio_irqnum].pid);
                        blink_led( runled_gpio_num, 1 );
                        send_sig(SIGPOLL, p, 0);
                }
                else if(reset_gpio_num == wps_gpio_num)
                {
                        printk(KERN_NOTICE NAME ": sending a SIGUSR1 to process %d\n",
                                                ralink_gpio_info[ralink_gpio_irqnum].pid);
                        /* WPS Button signal */
                        led_onoff( runled_gpio_num, 1);
                        send_sig(SIGPWR, p, 0);
                }
        }
        else if (usr == wps_gpio_num)
        {
                printk(KERN_NOTICE NAME ": sending a SIGUSR1 to process %d\n",
                                        ralink_gpio_info[ralink_gpio_irqnum].pid);
                /* WPS Button signal */
                led_onoff( runled_gpio_num, 1);
                send_sig(SIGPWR, p, 0);
        }

#else
	if (usr == 1) {
		printk(KERN_NOTICE NAME ": sending a SIGUSR1 to process %d\n",
				ralink_gpio_info[ralink_gpio_irqnum].pid);
		send_sig(SIGUSR1, p, 0);
	}
	else if (usr == 2) {
		printk(KERN_NOTICE NAME ": sending a SIGUSR2 to process %d\n",
				ralink_gpio_info[ralink_gpio_irqnum].pid);
		send_sig(SIGUSR2, p, 0);
	}
#endif
}

/*
 * 1. save the PIOINT and PIOEDGE value
 * 2. clear PIOINT by writing 1
 * (called by interrupt handler)
 */
void ralink_gpio_save_clear_intp(void)
{
	ralink_gpio_intp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIOINT));
	ralink_gpio_edge = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIOEDGE));
	
#if defined (RALINK_GPIO_HAS_9532)	
	*(volatile u32 *)(RALINK_REG_PIOINT) = cpu_to_le32(0xFFFFFFFF);
	*(volatile u32 *)(RALINK_REG_PIOEDGE) = cpu_to_le32(0xFFFFFFFF);
#else
	*(volatile u32 *)(RALINK_REG_PIOINT) = cpu_to_le32(0x00FFFFFF);
	*(volatile u32 *)(RALINK_REG_PIOEDGE) = cpu_to_le32(0x00FFFFFF);
#endif
#if defined (RALINK_GPIO_HAS_2722)
	ralink_gpio2722_intp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO2722INT));
	ralink_gpio2722_edge = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO2722EDGE));
	*(volatile u32 *)(RALINK_REG_PIO2722INT) = cpu_to_le32(0x0000FFFF);
	*(volatile u32 *)(RALINK_REG_PIO2722EDGE) = cpu_to_le32(0x0000FFFF);
#elif defined (RALINK_GPIO_HAS_4524)
	ralink_gpio3924_intp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO3924INT));
	ralink_gpio3924_edge = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO3924EDGE));
	*(volatile u32 *)(RALINK_REG_PIO3924INT) = cpu_to_le32(0x0000FFFF);
	*(volatile u32 *)(RALINK_REG_PIO3924EDGE) = cpu_to_le32(0x0000FFFF);
	ralink_gpio4540_intp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO4540INT));
	ralink_gpio4540_edge = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO4540EDGE));
	*(volatile u32 *)(RALINK_REG_PIO4540INT) = cpu_to_le32(0x00000FFF);
	*(volatile u32 *)(RALINK_REG_PIO4540EDGE) = cpu_to_le32(0x00000FFF);
#elif defined (RALINK_GPIO_HAS_5124)
	ralink_gpio3924_intp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO3924INT));
	ralink_gpio3924_edge = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO3924EDGE));
	*(volatile u32 *)(RALINK_REG_PIO3924INT) = cpu_to_le32(0x0000FFFF);
	*(volatile u32 *)(RALINK_REG_PIO3924EDGE) = cpu_to_le32(0x0000FFFF);
	ralink_gpio5140_intp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO5140INT));
	ralink_gpio5140_edge = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO5140EDGE));
	*(volatile u32 *)(RALINK_REG_PIO5140INT) = cpu_to_le32(0x00000FFF);
	*(volatile u32 *)(RALINK_REG_PIO5140EDGE) = cpu_to_le32(0x00000FFF);
#elif defined (RALINK_GPIO_HAS_9524) || defined (RALINK_GPIO_HAS_7224)
	ralink_gpio3924_intp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO3924INT));
	ralink_gpio3924_edge = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO3924EDGE));
	*(volatile u32 *)(RALINK_REG_PIO3924INT) = cpu_to_le32(0x0000FFFF);
	*(volatile u32 *)(RALINK_REG_PIO3924EDGE) = cpu_to_le32(0x0000FFFF);
	ralink_gpio7140_intp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO7140INT));
	ralink_gpio7140_edge = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO7140EDGE));
	*(volatile u32 *)(RALINK_REG_PIO7140INT) = cpu_to_le32(0xFFFFFFFF);
	*(volatile u32 *)(RALINK_REG_PIO7140EDGE) = cpu_to_le32(0xFFFFFFFF);
#if defined (RALINK_GPIO_HAS_7224)
	ralink_gpio72_intp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO72INT));
	ralink_gpio72_edge = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO72EDGE));
	*(volatile u32 *)(RALINK_REG_PIO72INT) = cpu_to_le32(0x00FFFFFF);
	*(volatile u32 *)(RALINK_REG_PIO72EDGE) = cpu_to_le32(0x00FFFFFF);
#else
	ralink_gpio9572_intp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO9572INT));
	ralink_gpio9572_edge = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO9572EDGE));
	*(volatile u32 *)(RALINK_REG_PIO9572INT) = cpu_to_le32(0x00FFFFFF);
	*(volatile u32 *)(RALINK_REG_PIO9572EDGE) = cpu_to_le32(0x00FFFFFF);
#endif
#elif defined (RALINK_GPIO_HAS_9532)
	ralink_gpio6332_intp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO6332INT));
	ralink_gpio6332_edge = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO6332EDGE));
	*(volatile u32 *)(RALINK_REG_PIO6332INT) = cpu_to_le32(0xFFFFFFFF);
	*(volatile u32 *)(RALINK_REG_PIO6332EDGE) = cpu_to_le32(0xFFFFFFFF);


	ralink_gpio9564_intp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO9564INT));
	ralink_gpio9564_edge = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO9564EDGE));
	*(volatile u32 *)(RALINK_REG_PIO9564INT) = cpu_to_le32(0xFFFFFFFF);
	*(volatile u32 *)(RALINK_REG_PIO9564EDGE) = cpu_to_le32(0xFFFFFFFF);

#endif
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
void ralink_gpio_irq_handler(unsigned int irq, struct irqaction *irqaction)
#else
irqreturn_t ralink_gpio_irq_handler(int irq, void *irqaction)
#endif
{
	struct gpio_time_record {
		unsigned long falling;
		unsigned long rising;
	};
	static struct gpio_time_record record[RALINK_GPIO_NUMBER];
	unsigned long now;
	int i;
	ralink_gpio_save_clear_intp();
	now = jiffies;
#if defined (RALINK_GPIO_HAS_2722)
	for (i = 0; i < 22; i++) {
		if (! (ralink_gpio_intp & (1 << i)))
			continue;
		ralink_gpio_irqnum = i;
		if (ralink_gpio_edge & (1 << i)) { //rising edge
			if (record[i].rising != 0 && time_before_eq(now,
						record[i].rising + 40L)) {
				/*
				 * If the interrupt comes in a short period,
				 * it might be floating. We ignore it.
				 */
			}
			else {
				record[i].rising = now;
				if (time_before(now, record[i].falling + 200L)) {
					//one click
					schedule_work(&gpio_event_click);
				}
				else {
					//press for several seconds
					schedule_work(&gpio_event_hold);
				}
			}
		}
		else { //falling edge
			record[i].falling = now;
		}
		break;
	}
	for (i = 22; i < 28; i++) {
		if (! (ralink_gpio2722_intp & (1 << (i - 22))))
			continue;
		ralink_gpio_irqnum = i;
		if (ralink_gpio2722_edge & (1 << (i - 22))) {
			if (record[i].rising != 0 && time_before_eq(now,
						record[i].rising + 40L)) {
			}
			else {
				record[i].rising = now;
				if (time_before(now, record[i].falling + 200L)) {
					schedule_work(&gpio_event_click);
				}
				else {
					schedule_work(&gpio_event_hold);
				}
			}
		}
		else {
			record[i].falling = now;
		}
		break;
	}
#elif defined (RALINK_GPIO_HAS_9532)
	for (i = 0; i < 32; i++) {
		if (! (ralink_gpio_intp & (1 << i)))
			continue;
			ralink_gpio_irqnum = i;
		if (ralink_gpio_edge & (1 << i)) { //rising edge
			if (record[i].rising != 0 && time_before_eq(now,
						record[i].rising + 40L)) {
				/*
				 * If the interrupt comes in a short period,
				 * it might be floating. We ignore it.
				 */
			}
			else {
				record[i].rising = now;
#ifdef CONFIG_EFM_PATCH
printk("ralink_gpio_notify_user => %d\n", i );
                                ralink_gpio_notify_user(i);
#else
				if (time_before(now, record[i].falling + 200L)) {
					//one click
					printk("one click\n");
					schedule_work(&gpio_event_click);
				}
				else {
					//press for several seconds
					printk("press for several seconds\n");
					schedule_work(&gpio_event_hold);
				}
#endif
			}
		}
		else { //falling edge
			 record[i].falling = now;
		}
		break;
	}
	for (i = 32; i < 64; i++) {
		if (! (ralink_gpio6332_intp & (1 << (i - 32))))
			continue;
		ralink_gpio_irqnum = i;
		if (ralink_gpio6332_edge & (1 << (i - 32))) {
			if (record[i].rising != 0 && time_before_eq(now,
						record[i].rising + 40L)) {
			}
			else {
				record[i].rising = now;
				if (time_before(now, record[i].falling + 200L)) {
					schedule_work(&gpio_event_click);
				}
				else {
					schedule_work(&gpio_event_hold);
				}
			}
		}
		else {
			record[i].falling = now;
		}
		break;
	}
	for (i = 64; i < RALINK_GPIO_NUMBER; i++) {
		if (! (ralink_gpio9564_intp & (1 << (i - 64))))
			continue;
		ralink_gpio_irqnum = i;
		if (ralink_gpio9564_edge & (1 << (i - 64))) {
			if (record[i].rising != 0 && time_before_eq(now,
						record[i].rising + 40L)) {
			}
			else {
				record[i].rising = now;
				if (time_before(now, record[i].falling + 200L)) {
					schedule_work(&gpio_event_click);
				}
				else {
					schedule_work(&gpio_event_hold);
				}
			}
		}
		else {
			record[i].falling = now;
		}
		break;
	}
#else
	for (i = 0; i < 24; i++) {
		if (! (ralink_gpio_intp & (1 << i)))
			continue;
		ralink_gpio_irqnum = i;
		if (ralink_gpio_edge & (1 << i)) { //rising edge
			if (record[i].rising != 0 && time_before_eq(now,
						record[i].rising + 40L)) {
				/*
				 * If the interrupt comes in a short period,
				 * it might be floating. We ignore it.
				 */
			}
			else {
				record[i].rising = now;
#ifdef CONFIG_EFM_PATCH
printk("ralink_gpio_notify_user => %d\n", i );
                                ralink_gpio_notify_user(i);
#else
				if (time_before(now, record[i].falling + 200L)) {
					//one click
					printk("i=%d, one click\n", i);
					schedule_work(&gpio_event_click);
				}
				else {
					//press for several seconds
					printk("i=%d, push several seconds\n", i);
					schedule_work(&gpio_event_hold);
				}
#endif
			}
		}
		else { //falling edge
			record[i].falling = now;
		}
		break;
	}
#if defined (RALINK_GPIO_HAS_4524)
	for (i = 24; i < 40; i++) {
		if (! (ralink_gpio3924_intp & (1 << (i - 24))))
			continue;
		ralink_gpio_irqnum = i;
		if (ralink_gpio3924_edge & (1 << (i - 24))) {
			if (record[i].rising != 0 && time_before_eq(now,
						record[i].rising + 40L)) {
			}
			else {
				record[i].rising = now;
				if (time_before(now, record[i].falling + 200L)) {
					schedule_work(&gpio_event_click);
				}
				else {
					schedule_work(&gpio_event_hold);
				}
			}
		}
		else {
			record[i].falling = now;
		}
		break;
	}
	for (i = 40; i < RALINK_GPIO_NUMBER; i++) {
		if (! (ralink_gpio4540_intp & (1 << (i - 40))))
			continue;
		ralink_gpio_irqnum = i;
		if (ralink_gpio4540_edge & (1 << (i - 40))) {
			if (record[i].rising != 0 && time_before_eq(now,
						record[i].rising + 40L)) {
			}
			else {
				record[i].rising = now;
				if (time_before(now, record[i].falling + 200L)) {
					schedule_work(&gpio_event_click);
				}
				else {
					schedule_work(&gpio_event_hold);
				}
			}
		}
		else {
			record[i].falling = now;
		}
		break;
	}
#elif defined (RALINK_GPIO_HAS_5124)
	for (i = 24; i < 40; i++) {
		if (! (ralink_gpio3924_intp & (1 << (i - 24))))
			continue;
		ralink_gpio_irqnum = i;
		if (ralink_gpio3924_edge & (1 << (i - 24))) {
			if (record[i].rising != 0 && time_before_eq(now,
						record[i].rising + 40L)) {
			}
			else {
				record[i].rising = now;
				if (time_before(now, record[i].falling + 200L)) {
					schedule_work(&gpio_event_click);
				}
				else {
					schedule_work(&gpio_event_hold);
				}
			}
		}
		else {
			record[i].falling = now;
		}
		break;
	}
	for (i = 40; i < RALINK_GPIO_NUMBER; i++) {
		if (! (ralink_gpio5140_intp & (1 << (i - 40))))
			continue;
		ralink_gpio_irqnum = i;
		if (ralink_gpio5140_edge & (1 << (i - 40))) {
			if (record[i].rising != 0 && time_before_eq(now,
						record[i].rising + 40L)) {
			}
			else {
				record[i].rising = now;
				if (time_before(now, record[i].falling + 200L)) {
					schedule_work(&gpio_event_click);
				}
				else {
					schedule_work(&gpio_event_hold);
				}
			}
		}
		else {
			record[i].falling = now;
		}
		break;
	}
#elif defined (RALINK_GPIO_HAS_9524) || defined (RALINK_GPIO_HAS_7224)
	for (i = 24; i < 40; i++) {
		if (! (ralink_gpio3924_intp & (1 << (i - 24))))
			continue;
		ralink_gpio_irqnum = i;
		if (ralink_gpio3924_edge & (1 << (i - 24))) {
			if (record[i].rising != 0 && time_before_eq(now,
						record[i].rising + 40L)) {
			}
			else {
				record[i].rising = now;
				if (time_before(now, record[i].falling + 200L)) {
					printk("i=%d, one click\n", i);
					schedule_work(&gpio_event_click);
				}
				else {
					printk("i=%d, push several seconds\n", i);
					schedule_work(&gpio_event_hold);
				}
			}
		}
		else {
			record[i].falling = now;
		}
		break;
	}
	for (i = 40; i < 72; i++) {
		if (! (ralink_gpio7140_intp & (1 << (i - 40))))
			continue;
		ralink_gpio_irqnum = i;
		if (ralink_gpio7140_edge & (1 << (i - 40))) {
			if (record[i].rising != 0 && time_before_eq(now,
						record[i].rising + 40L)) {
			}
			else {
				record[i].rising = now;
				if (time_before(now, record[i].falling + 200L)) {
					printk("i=%d, one click\n", i);
					schedule_work(&gpio_event_click);
				}
				else {
					printk("i=%d, push several seconds\n", i);
					schedule_work(&gpio_event_hold);
				}
			}
		}
		else {
			record[i].falling = now;
		}
		break;
	}
#if defined (RALINK_GPIO_HAS_7224)
	for (i = 72; i < RALINK_GPIO_NUMBER; i++) {
		if (! (ralink_gpio72_intp & (1 << (i - 72))))
			continue;
		ralink_gpio_irqnum = i;
		if (ralink_gpio72_edge & (1 << (i - 72))) {
			if (record[i].rising != 0 && time_before_eq(now,
						record[i].rising + 40L)) {
			}
			else {
				record[i].rising = now;
				if (time_before(now, record[i].falling + 200L)) {
					printk("i=%d, one click\n", i);
					schedule_work(&gpio_event_click);
				}
				else {
					printk("i=%d, push several seconds\n", i);
					schedule_work(&gpio_event_hold);
				}
			}
		}
		else {
			record[i].falling = now;
		}
		break;
	}
#else
	for (i = 72; i < RALINK_GPIO_NUMBER; i++) {
		if (! (ralink_gpio9572_intp & (1 << (i - 72))))
			continue;
		ralink_gpio_irqnum = i;
		if (ralink_gpio9572_edge & (1 << (i - 72))) {
			if (record[i].rising != 0 && time_before_eq(now,
						record[i].rising + 40L)) {
			}
			else {
				record[i].rising = now;
				if (time_before(now, record[i].falling + 200L)) {
					printk("i=%d, one click\n", i);
					schedule_work(&gpio_event_click);
				}
				else {
					printk("i=%d, push several seconds\n", i);
					schedule_work(&gpio_event_hold);
				}
			}
		}
		else {
			record[i].falling = now;
		}
		break;
	}
#endif
#endif
#endif

	return IRQ_HANDLED;
}

struct irqaction ralink_gpio_irqaction = {
	.handler = ralink_gpio_irq_handler,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,35)
	.flags = IRQF_DISABLED,
#else
	.flags = SA_INTERRUPT,
#endif
	.name = "ralink_gpio",
};

void __init ralink_gpio_init_irq(void)
{
	setup_irq(SURFBOARDINT_GPIO, &ralink_gpio_irqaction);
}

module_init(ralink_gpio_init);
module_exit(ralink_gpio_exit);

