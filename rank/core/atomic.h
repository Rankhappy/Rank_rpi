/*
*Name: type.c
*Author: Rank 
*Contact:<441552318@qq.com>
*/

#ifndef ATOMIC_H
#define ATOMIC_H

typedef struct
{
	volatile int value;
}atomic_t;

/*Will implement later.*/
#define irq_disable()
#define irq_restore()

#define atomic_inc(a)  atomic_add(a, 1)
#define atomic_dec(a)  atomic_sub(a, 1)

extern void cpu_lock(void);
extern void cpu_unlock(void);

static void atomic_set(atomic_t *a, int v)
{
	irq_disable();

	cpu_lock();
	a->value = v;
	cpu_unlock();
	
	irq_restore();
}

static int atomic_get(atomic_t *a)
{
	int v;

	irq_disable();

	cpu_lock();
	v = a->value;
	cpu_unlock();
	
	irq_restore();

	return v;
}

static int atomic_get_set(atomic_t *a, int new_v)
{
	int old_v;

	irq_disable();

	cpu_lock();
	old_v = a->value;
	a->value = new_v;
	cpu_unlock();
	
	irq_restore();

	return old_v;
}

static void atomic_add(atomic_t *a, int v)
{
	irq_disable();

	cpu_lock();
	a->value += v;
	cpu_unlock();
	
	irq_restore();
}

static void atomic_sub(atomic_t *a, int v)
{
	irq_disable();

	cpu_lock();
	a->value -= v;
	cpu_unlock();
	
	irq_restore();
}

#endif

