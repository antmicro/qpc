/**
* @file
* @brief "Experimental" QF/C port to Espressif ESP-IDF (version 4.x)
* @ingroup ports
* @cond
******************************************************************************
* Last updated for version 6.9.4
* Last updated on  2021-06-29
*
*                    Q u a n t u m  L e a P s
*                    ------------------------
*                    Modern Embedded Software
*
* Copyright (C) 2005-2021 Quantum Leaps, LLC. All rights reserved.
*
* This program is open source software: you can redistribute it and/or
* modify it under the terms of the GNU General Public License as published
* by the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* Alternatively, this program may be distributed and modified under the
* terms of Quantum Leaps commercial licenses, which expressly supersede
* the GNU General Public License and are specifically designed for
* licensees interested in retaining the proprietary status of their code.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program. If not, see <www.gnu.org/licenses>.
*
* Contact information:
* <www.state-machine.com/licensing>
* <info@state-machine.com>
******************************************************************************
* @endcond
*/
#ifndef QF_PORT_H
#define QF_PORT_H

/* FreeRTOS-ESP32 event queue and thread types, see NOTE0 */
#define QF_EQUEUE_TYPE        QEQueue
#define QF_THREAD_TYPE        StaticTask_t

/* The maximum number of active objects in the application, see NOTE1 */
#define QF_MAX_ACTIVE         8U

/* QF critical section for FreeRTOS-ESP32 (task level), see NOTE2 */
/* #define QF_CRIT_STAT_TYPE not defined */
#define QF_CRIT_ENTRY(dummy)  portENTER_CRITICAL(&QF_esp32mux)
#define QF_CRIT_EXIT(dummy)   portEXIT_CRITICAL(&QF_esp32mux)

#include "freertos/FreeRTOS.h"  /* FreeRTOS master include file, see NOTE4 */
#include "freertos/task.h"      /* FreeRTOS task  management */

#include "qep_port.h"  /* QEP port */
#include "qequeue.h"   /* this QP port uses the native QF event queue */
#include "qmpool.h"    /* this QP port uses the native QF memory pool */
#include "qf.h"        /* QF platform-independent public interface */

/* global spinlock "mutex" for all critical sections in QF (see NOTE3) */
extern PRIVILEGED_DATA portMUX_TYPE QF_esp32mux;

/* the "FromISR" versions of the QF APIs, see NOTE4 */
#ifdef Q_SPY
    #define QACTIVE_POST_FROM_ISR(me_, e_, pxHigherPrioTaskWoken_, sender_) \
        ((void)QActive_postFromISR_((me_), (e_), QF_NO_MARGIN,              \
                                    (pxHigherPrioTaskWoken_), (sender_)))

    #define QACTIVE_POST_X_FROM_ISR(me_, e_, margin_,                \
                                    pxHigherPrioTaskWoken_, sender_) \
        (QActive_postFromISR_((me_), (e_), (margin_),                \
                              (pxHigherPrioTaskWoken_), (sender_)))

    #define QF_PUBLISH_FROM_ISR(e_, pxHigherPrioTaskWoken_, sender_) \
        (QF_publishFromISR_((e_), (pxHigherPrioTaskWoken_),          \
                            (void const *)(sender_)))

    #define QF_TICK_X_FROM_ISR(tickRate_, pxHigherPrioTaskWoken_, sender_) \
        (QF_tickXFromISR_((tickRate_), (pxHigherPrioTaskWoken_), (sender_)))

    /* this function only to be used through macros QACTIVE_POST_FROM_ISR()
    * and QACTIVE_POST_X_FROM_ISR().
    */
    bool QActive_postFromISR_(QActive * const me, QEvt const * const e,
                              uint_fast16_t const margin,
                              BaseType_t * const pxHigherPriorityTaskWoken,
                              void const * const sender);

    void QF_publishFromISR_(QEvt const * const e,
                              BaseType_t * const pxHigherPriorityTaskWoken,
                              void const * const sender);

    void QF_tickXFromISR_(uint_fast8_t const tickRate,
                              BaseType_t * const pxHigherPriorityTaskWoken,
                              void const * const sender);
#else
    #define QACTIVE_POST_FROM_ISR(me_, e_, pxHigherPrioTaskWoken_, dummy) \
        ((void)QActive_postFromISR_((me_), (e_), QF_NO_MARGIN,           \
                                    (pxHigherPrioTaskWoken_)))

    #define QACTIVE_POST_X_FROM_ISR(me_, e_, margin_,               \
                                    pxHigherPrioTaskWoken_,  dummy) \
        (QActive_postFromISR_((me_), (e_), (margin_),               \
                              (pxHigherPrioTaskWoken_)))

    #define QF_PUBLISH_FROM_ISR(e_, pxHigherPrioTaskWoken_, dummy) \
        (QF_publishFromISR_((e_), (pxHigherPrioTaskWoken_)))

    #define QF_TICK_X_FROM_ISR(tickRate_, pxHigherPrioTaskWoken_, dummy) \
        (QF_tickXFromISR_((tickRate_), (pxHigherPrioTaskWoken_)))

    bool QActive_postFromISR_(QActive * const me, QEvt const * const e,
                              uint_fast16_t const margin,
                              BaseType_t * const pxHigherPriorityTaskWoken);

    void QF_publishFromISR_(QEvt const * const e,
                              BaseType_t * const pxHigherPriorityTaskWoken);

    void QF_tickXFromISR_(uint_fast8_t const tickRate,
                              BaseType_t * const pxHigherPriorityTaskWoken);

#endif

#define QF_TICK_FROM_ISR(pxHigherPrioTaskWoken_, sender_) \
    QF_TICK_X_FROM_ISR(0U, pxHigherPrioTaskWoken_, sender_)

#ifdef Q_EVT_CTOR /* Shall the ctor for the ::QEvt class be provided? */

    #define Q_NEW_FROM_ISR(evtT_, sig_, ...)                  \
        (evtT_##_ctor((evtT_ *)QF_newXFromISR_(sizeof(evtT_), \
                      QF_NO_MARGIN, 0), (sig_), ##__VA_ARGS__))

    #define Q_NEW_X_FROM_ISR(e_, evtT_, margin_, sig_, ...) do { \
        (e_) = (evtT_ *)QF_newXFromISR_(sizeof(evtT_),           \
                                 (margin_), 0);                  \
        if ((e_) != (evtT_ *)0) {                                \
            evtT_##_ctor((e_), (sig_), ##__VA_ARGS__);           \
        }                                                        \
     } while (false)

#else

    #define Q_NEW_FROM_ISR(evtT_, sig_)                         \
        ((evtT_ *)QF_newXFromISR_((uint_fast16_t)sizeof(evtT_), \
                                  QF_NO_MARGIN, (sig_)))

    #define Q_NEW_X_FROM_ISR(e_, evtT_, margin_, sig_) ((e_) = \
        (evtT_ *)QF_newXFromISR_((uint_fast16_t)sizeof(evtT_), \
                                 (margin_), (sig_)))

#endif /* Q_EVT_CTOR */

void QF_gcFromISR(QEvt const * const e);

/* this function only to be used through macros Q_NEW_FROM_ISR() and
* Q_NEW_X_FROM_ISR().
*/
QEvt *QF_newXFromISR_(uint_fast16_t const evtSize,
                      uint_fast16_t const margin, enum_t const sig);

void *QMPool_getFromISR(QMPool * const me, uint_fast16_t const margin,
                        uint_fast8_t const qs_id);
void QMPool_putFromISR(QMPool * const me, void *b, uint_fast8_t const qs_id);

enum FreeRTOS_TaskAttrs {
    TASK_NAME_ATTR
};

/* FreeRTOS hooks prototypes (not provided by FreeRTOS) */
#if (configUSE_IDLE_HOOK > 0)
    void vApplicationIdleHook(void);
#endif
#if (configUSE_TICK_HOOK > 0)
    void vApplicationTickHook(void);
#endif
#if (configCHECK_FOR_STACK_OVERFLOW > 0)
    void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName);
#endif
#if (configSUPPORT_STATIC_ALLOCATION > 0)
    void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer,
                                        StackType_t **ppxIdleTaskStackBuffer,
                                        uint32_t *pulIdleTaskStackSize );
#endif

/*****************************************************************************
* interface used only inside QF, but not in applications
*/
#ifdef QP_IMPL
    /* FreeRTOS blocking for event queue implementation (task level) */
    #define QACTIVE_EQUEUE_WAIT_(me_)                 \
        while ((me_)->eQueue.frontEvt == (QEvt *)0) { \
            QF_CRIT_X_();                             \
            ulTaskNotifyTake(pdTRUE, portMAX_DELAY);  \
            QF_CRIT_E_();                             \
        }

    /* FreeRTOS signaling (unblocking) for event queue (task level) */
    #define QACTIVE_EQUEUE_SIGNAL_(me_) do {           \
        QF_CRIT_X_();                                  \
        xTaskNotifyGive((TaskHandle_t)&(me_)->thread); \
        QF_CRIT_E_();                                  \
    } while (false)

    #define QF_SCHED_STAT_
    #define QF_SCHED_LOCK_(dummy) vTaskSuspendAll()
    #define QF_SCHED_UNLOCK_()    xTaskResumeAll()

    /* native QF event pool operations */
    #define QF_EPOOL_TYPE_            QMPool
    #define QF_EPOOL_INIT_(p_, poolSto_, poolSize_, evtSize_) \
        (QMPool_init(&(p_), (poolSto_), (poolSize_), (evtSize_)))
    #define QF_EPOOL_EVENT_SIZE_(p_)  ((uint_fast16_t)(p_).blockSize)
    #define QF_EPOOL_GET_(p_, e_, m_, qs_id_) \
        ((e_) = (QEvt *)QMPool_get(&(p_), (m_), (qs_id_)))
    #define QF_EPOOL_PUT_(p_, e_, qs_id_) \
        (QMPool_put(&(p_), (e_), (qs_id_)))

#endif /* ifdef QP_IMPL */

/*****************************************************************************
* NOTE0:
* This is the "experimental" port to the [Espressif ESP-IDF][1]
* IoT Framework, which is loosely based on the [FreeRTOS kernel][2].
*
* "Experimental" means that the port has not been thouroughly tested at
* Quantum Leaps and no working examples are provided.
*
* The [Espressif ESP-IDF][1] is based on a significantly changed version
* of the FreeRTOS kernel developed by Espressif to support the ESP32 multi-core
* CPUs (see [ESP-IDF][1]).
*
* The Espressif version of FreeRTOS is __NOT__ compatible with the baseline
* FreeRTOS and it needs to be treated as a separate RTOS kernel.
* According to the comments in the Espressif source code, FreeRTOS-ESP-IDF
* is based on FreeRTOS V8.2.0, but apparently FreeRTOS-ESP-IDF has been
* updated with the newer features introduced to the original FreeRTOS in the
* later versions. For example, FreeRTOS-ESP32 supports the "static allocation",
* first introduced in baseline FreeRTOS V9.x. This QP port to FreeRTOS-ESP-IDF
* takes advantage of the "static allocation".
*
* [1]: https://www.espressif.com/en/products/sdks/esp-idf
* [2]: https://freertos.org
*
* NOTE1:
* The maximum number of active objects QF_MAX_ACTIVE can be increased to 64,
* inclusive, but it can be reduced to save some memory. Also, the number of
* active objects cannot exceed the number of FreeRTOS task priorities,
* because each QP active object requires a unique priority level.
*
* NOTE2:
* The critical section definition applies only to the FreeRTOS "task level"
* APIs. The "FromISR" APIs are defined separately.
*
* NOTE3:
* This QF port to FreeRTOS-ESP32 uses the FreeRTOS-ESP32 spin lock "mutex",
* similar to the internal implementation of FreeRTOS-ESP32 (see tasks.c).
* However, the QF port uses its own "mutex" object QF_esp32mux.
*
* NOTE4:
* The design of FreeRTOS requires using different APIs inside the ISRs
* (the "FromISR" variant) than at the task level. Accordingly, this port
* provides the "FromISR" variants for QP functions and "FROM_ISR" variants
* for QP macros to be used inside ISRs. ONLY THESE "FROM_ISR" VARIANTS
* ARE ALLOWED INSIDE ISRs AND CALLING THE TASK-LEVEL APIs IS AN ERROR.
*/

#endif /* QF_PORT_H */
