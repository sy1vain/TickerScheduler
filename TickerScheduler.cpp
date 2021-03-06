#include "TickerScheduler.h"

TickerScheduler::TickerScheduler(uint8_t size)
{
    this->items = new TickerSchedulerItem[size];
    this->size = size;
}

TickerScheduler::~TickerScheduler()
{
    for (uint8_t i = 0; i < this->size; i++)
    {
        this->remove(i);
        yield();
    }

    delete[] this->items;
    this->items = NULL;
    this->size = 0;
}

void TickerScheduler::handleTickerFlag(volatile bool * flag)
{
	if (!*flag)
		*flag = true;
}

void TickerScheduler::handleTicker(tscallback_t f, void * arg, volatile bool * flag)
{
    if (*flag)
    {
        yield();
        *flag = false;
        yield();
        f(arg);
        yield();
    }
}

void TickerScheduler::handleTicker(TickerSchedulerItem * item)
{
    if(item->flag)
    {
        yield();
        item->flag = false;
        yield();
        if(!item->repeat) item->is_used = false;
        yield();
        item->cb(item->cb_arg);
        yield();
    }
}

bool TickerScheduler::add(uint8_t i, uint32_t period, tscallback_t f, void* arg, boolean repeat)
{
    if (i >= this->size || this->items[i].is_used)
        return false;

    this->items[i].cb = f;
    this->items[i].cb_arg = arg;
    this->items[i].flag = false;
    this->items[i].period = period;
    this->items[i].is_used = true;
    this->items[i].repeat = repeat;

    enable(i);

    return true;
}

bool TickerScheduler::once(uint8_t i, uint32_t period, tscallback_t f, void* arg)
{
    return add(i, period, f, arg, false);
}

bool TickerScheduler::remove(uint8_t i)
{
    if (i >= this->size || !this->items[i].is_used)
        return false;

    this->items[i].is_used = false;
    this->items[i].t.detach();
    this->items[i].flag = false;
    this->items[i].cb = NULL;

    return true;
}

bool TickerScheduler::disable(uint8_t i)
{
    if (i >= this->size || !this->items[i].is_used)
        return false;

    this->items[i].t.detach();

    return true;
}

bool TickerScheduler::enable(uint8_t i)
{
    if (i >= this->size || !this->items[i].is_used)
        return false;

	volatile bool * flag = &this->items[i].flag;
    if(this->items[i].repeat){
	   this->items[i].t.attach_ms(this->items[i].period, TickerScheduler::handleTickerFlag, flag);
    }else{
        this->items[i].t.once_ms(this->items[i].period, TickerScheduler::handleTickerFlag, flag);
    }

    return true;
}

void TickerScheduler::disableAll()
{
    for (uint8_t i = 0; i < this->size; i++)
        disable(i);
}

void TickerScheduler::enableAll()
{
    for (uint8_t i = 0; i < this->size; i++)
        enable(i);
}

void TickerScheduler::update()
{
    for (uint8_t i = 0; i < this->size; i++)
    {
		if (this->items[i].is_used)
		{
			#ifdef ARDUINO_ARCH_AVR
			this->items[i].t.Tick();
			#endif

			handleTicker(&this->items[i]);
		}
        yield();
    }
}
