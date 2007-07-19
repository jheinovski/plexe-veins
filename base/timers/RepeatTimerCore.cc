#include "RepeatTimerCore.h"
#include "RepeatTimer.h"

#include <assert.h>

Define_Module_Like(RepeatTimerCore,Trivial);

void RepeatTimerCore::checkExists(unsigned int index)
{
	if (timer_map->find(index)==timer_map->end())
		error(" timer index %u doesn't exist", index);
}

void RepeatTimerCore::handleMessage(cMessage* msg)
{
	assert(msg->isSelfMessage());
	unsigned int index = msg->kind();
	(*timer_map)[index].count --;
	if ((*timer_map)[index].count > 0) {
		cMessage *timer = (*timer_map)[index].timer;
		double when = (*timer_map)[index].when;
		scheduleAt(simTime() + when, timer);
	}
	simulation.setContextModule(timer->owner);	
	timer->handleRepeatTimer(msg->kind());
}

void RepeatTimerCore::init(RepeatTimer *owner)
{
	timer = owner;
// 	timers = new std::map<unsigned int,cMessage *>();
// 	destructors = new std::map<unsigned int,cleanup *>();
	timer_map = new std::map<unsigned int,TInfo>();
}

unsigned int RepeatTimerCore::setRepeatTimer(double when, int repeats)
{
	unsigned int key = timer_map->size();
	while (timer_map->find(key)!=timer_map->end())
		key++;
	setRepeatTimer(key,when,repeats);
	return key;
}

void RepeatTimerCore::setRepeatTimer(unsigned int index, double when, int repeats)
{
	Enter_Method_Silent();
	cMessage *timer;
	if (timer_map->find(index)==timer_map->end())
	{
		timer = new cMessage("timer");
		timer->setKind(index);
		(*timer_map)[index].timer = timer;
		(*timer_map)[index].destructor = NULL;
	}
	else
	{
		timer = (*timer_map)[index].timer;
		if (timer->isScheduled())
			cancelEvent(timer);
	}
	(*timer_map)[index].count = (*timer_map)[index].repeats = repeats;
	(*timer_map)[index].when = when;

	scheduleAt(simTime() + when, timer);	
}

void RepeatTimerCore::cancelRepeatTimer(unsigned int index)
{
	Enter_Method_Silent();
	checkExists(index);
	if ((*timer_map)[index].timer->isScheduled())
		cancelEvent((*timer_map)[index].timer);
}

float RepeatTimerCore::remainingRepeatTimer(unsigned int index)
{
	checkExists(index);
	if ((*timer_map)[index].timer->isScheduled())
		return (*timer_map)[index].timer->arrivalTime()-simTime();
	else
		return -1;
}

void RepeatTimerCore::setContextDestructor(unsigned int index, cleanup *c)
{
	checkExists(index);
	(*timer_map)[index].destructor = c;
}

RepeatTimerCore::~RepeatTimerCore()
{
	for (std::map<unsigned int,TInfo>::const_iterator p=timer_map->begin();p!=timer_map->end();p++)
	{
		unsigned int index = p->second.timer->kind();
		checkExists(index);
		if ((*timer_map)[index].timer->isScheduled())
		{
			if ((*timer_map)[index].destructor!=NULL)
				(*timer_map)[index].destructor(contextPointer(index));
			cancelEvent((*timer_map)[index].timer);
		}
		delete p->second.timer;
	}
// 	delete timers;
// 	delete destructors;
	delete timer_map;
}

/** Set a "context pointer" refering to some piece of opaque useful data
 * @param index RepeatTimer number
 * @param data Opaque pointer. Never free'd or dereferenced
 */
void RepeatTimerCore::setContextPointer(unsigned int index,void * data)
{
	checkExists(index);
	(*timer_map)[index].timer->setContextPointer(data);
}

/** Retreive a "context pointer" refering to some piece of opaque useful data
 * @param index RepeatTimer number
 * @return Opaque pointer from @setContextPointer
 */
void * RepeatTimerCore::contextPointer(unsigned int index)
{
	checkExists(index);
	return (*timer_map)[index].timer->contextPointer();
}

/* Mark the first @count pointer ids (from 0 to @count-1) as allocated, so they don't get
 * auto-allocated by setRepeatTimer
 * @param count Number of timers to allocate
 */
void RepeatTimerCore::allocateRepeatTimers(unsigned int count)
{
	Enter_Method_Silent();
	for (unsigned int i=0;i<count;i++)
	{
		cMessage *timer;
		if (timer_map->find(i)==timer_map->end())
		{
			timer = new cMessage("timer");
			timer->setKind(i);
			(*timer_map)[i].timer = timer;
		}
	}
}

/* Delete a timer. Useful for auto-allocated timers that you don't need any more to 
 * reduce memory usage. Does nothing if the timer doesn't exist
 * @param index RepeatTimer to wipe
 */
void RepeatTimerCore::deleteRepeatTimer(unsigned int index)
{
	if (timer_map->find(index)!=timer_map->end())
	{
		cancelRepeatTimer(index);
		delete timer_map->find(index)->second.timer;
		timer_map->erase(timer_map->find(index));
	}
}

void RepeatTimerCore::resetRepeatTimer(unsigned int index)
{
	Enter_Method_Silent();
	if (timerExists(index)) {
		(*timer_map)[index].count = (*timer_map)[index].repeats;

		cMessage *timer = (*timer_map)[index].timer;
		if (timer->isScheduled())
			cancelEvent(timer);
		double when = (*timer_map)[index].when;
		scheduleAt(simTime() + when, timer);
	}
}

void RepeatTimerCore::resetAllRepeatTimers(void)
{
	std::map <unsigned int, TInfo>::const_iterator p;
	for (p = timer_map->begin(); p != timer_map->end(); p++) {
		
		resetRepeatTimer(p->second.timer->kind());
	}
}

bool RepeatTimerCore::timerExists(unsigned int index)
{
	return timer_map->find(index) != timer_map->end();
}

