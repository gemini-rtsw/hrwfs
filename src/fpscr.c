#include <vxWorks.h>
#include <stdio.h>
#include <vxLib.h>

#include "taskLib.h"

#include "arch/ppc/archPpc.h"
#include "arch/ppc/fppPpcLib.h"

/*
**  The following #define statements allow control over which floating-point
**  conditions generate exceptions.  Use FP_ENABLE_... to explicitly enable an
**  exception and FP_DISABLE_... to explicilty disable it.  Use #undef to use
**  the default VxWorks setting for a given condtition.
**
**  The recommended configuration enables exceptions for invalid operations,
**  overflows, and division by zero, and disables exceptions for underflows and
**  inexact conditions.
*/
#define	FP_DISABLE_INVALID_OPERATION_EXCEPTIONS
#define	FP_ENABLE_OVERFLOW_EXCEPTIONS
#define	FP_DISABLE_UNDERFLOW_EXCEPTIONS
#define	FP_ENABLE_ZERO_DIVIDE_EXCEPTIONS
#define	FP_DISABLE_INEXACT_EXCEPTIONS

/*
**  The following #define statement allows control over the floating-point
**  non-IEEE mode.  In non-IEEE mode, denormalized numbers, NaNs, and some IEEE
**  invalid operations are treated in a non-IEEE conforming manner.  Refer to
**  the PowerPC 604 RISC Microprocessor User's Manual for details.  Use
**  FP_ENABLE_... to explicitly enable non-IEEE mode and FP_DISABLE_... to
**  explicitly disable it.  Use #undef to use the default VxWorks setting.
**
**  The recommended configuration disables non-IEEE mode (i.e., all floating-
**  point operations are IEEE compliant).  Use of non-IEEE mode causes
**  denormalized operands and results to become zero, which may be desirable
**  for code ported from platforms using DEC proprietary floating-point
**  formats.
*/
#define	FP_DISABLE_NON_IEEE_MODE

/*
**  The following #define statement defines precise mode for floating-point
**  exceptions.  In this mode, the system floating-point enabled exception
**  handler is invoked precisely at the instruction that caused the exception.
**  Use #define to enable this mode and #undef to disable it.  Note that
**  precise mode will not be set if no floating-point exceptions are enabled.
**
**  The recommended configuration disables precise mode.  Precise mode may
**  degrade performance and should be used only for debugging.
*/
#undef	FP_PRECISE_MODE


/*#ifdef	INCLUDE_HW_FP */
/*******************************************************************************
*
* usrFppCreateHook - modify floating-point environment for a newly-created task
*
* This routine modifies the default VxWorks floating-point environment for a
* newly-created task.  Floating-point exceptions are explicitly disabled for
* tasks created without floating-point support.  The floating-point environment
* is established as specified by the user for tasks created with floating-point
* support.  usrFppCreateHook is installed as a task create hook and is called
* indirectly whenever a new task is created.
*
* RETURNS: N/A
*/

void usrFppCreateHook(FAST WIND_TCB *pTcb)
{

#if (CPU_FAMILY == PPC)
    int fpcsr;	/* floating-point status and control register, alias fpscr */

    int fpcsrOld=0;
/*
**  If floating-point operations are not enabled for this task, set the FE bits
**  to ignore floating-point exceptions, and return.
*/
    if (!(pTcb->options & VX_FP_TASK))
    {
	pTcb->regs.msr &= ~(_PPC_MSR_FE0 | _PPC_MSR_FE1);
	return;
    }

/*
**  Modify the the floating-point status and control register in the floating-
**  point context of the new task.
*/
    fpcsr = (int)(FP_CONTEXT *)(pTcb->pFpContext)->fpcsr;
    fpcsrOld = fpcsr;
    
#if defined(FP_ENABLE_INVALID_OPERATION_EXCEPTIONS)
    fpcsr |= _PPC_FPSCR_VE;
#elif defined(FP_DISABLE_INVALID_OPERATION_EXCEPTIONS)
    fpcsr &= ~_PPC_FPSCR_VE;
#endif

#if defined(FP_ENABLE_OVERFLOW_EXCEPTIONS)
    fpcsr |= _PPC_FPSCR_OE;
#elif defined(FP_DISABLE_OVERFLOW_EXCEPTIONS)
    fpcsr &= ~_PPC_FPSCR_OE;
#endif

#if defined(FP_ENABLE_UNDERFLOW_EXCEPTIONS)
    fpcsr |= _PPC_FPSCR_UE;
#elif defined(FP_DISABLE_UNDERFLOW_EXCEPTIONS)
    fpcsr &= ~_PPC_FPSCR_UE;
#endif

#if defined(FP_ENABLE_ZERO_DIVIDE_EXCEPTIONS)
    fpcsr |= _PPC_FPSCR_ZE;
#elif defined(FP_DISABLE_ZERO_DIVIDE_EXCEPTIONS)
    fpcsr &= ~_PPC_FPSCR_ZE;
#endif

#if defined(FP_ENABLE_INEXACT_EXCEPTIONS)
    fpcsr |= _PPC_FPSCR_XE;
#elif defined(FP_DISABLE_INEXACT_EXCEPTIONS)
    fpcsr &= ~_PPC_FPSCR_XE;
#endif

#if defined(FP_ENABLE_NON_IEEE_MODE)
    fpcsr |= _PPC_FPSCR_NI;
#elif defined(FP_DISABLE_NON_IEEE_MODE)
    fpcsr &= ~_PPC_FPSCR_NI;
#endif

    pTcb->pFpContext->fpcsr = fpcsr;

/*
**  Modify the machine status register (MSR) in the context of the new task.
**  If no floating-point exceptions are enabled, set the FE bits to ignore
**  floating-point exceptions.  Otherwise, set precise mode, if defined above,
**  or imprecise nonrecoverable mode.
*/
    if (!(fpcsr & (_PPC_FPSCR_VE | _PPC_FPSCR_OE | _PPC_FPSCR_UE |
		   _PPC_FPSCR_ZE | _PPC_FPSCR_XE)))
	pTcb->regs.msr &= ~(_PPC_MSR_FE0 | _PPC_MSR_FE1);
    else
#ifdef FP_PRECISE_MODE
	pTcb->regs.msr |= _PPC_MSR_FE0 | _PPC_MSR_FE1;
#else
	pTcb->regs.msr = (pTcb->regs.msr & ~_PPC_MSR_FE0) | _PPC_MSR_FE1;
#endif
#endif /* CPU_FAMILY == PPC */

}
/* #endif */ /* INCLUDE_HW_FP */

