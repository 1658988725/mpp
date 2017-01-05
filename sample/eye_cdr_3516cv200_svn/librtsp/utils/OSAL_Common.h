#ifndef OMX_COMMON
#define OMX_COMMON
typedef void* OMX_HANDLETYPE;

typedef enum OMX_ERRORTYPE
{
  OMX_ErrorNone = 0,

  /** There were insufficient resources to perform the requested operation */
  OMX_ErrorInsufficientResources = 0x80001000,

  /** There was an error, but the cause of the error could not be determined */
  OMX_ErrorUndefined = 0x80001001,

  /** The component name string was not valid */
  OMX_ErrorInvalidComponentName = 0x80001002,

  /** No component with the specified name string was found */
  OMX_ErrorComponentNotFound = 0x80001003,

  /** The component specified did not have a "OMX_ComponentInit" or
      "OMX_ComponentDeInit entry point */
  OMX_ErrorInvalidComponent = 0x80001004,

  /** One or more parameters were not valid */
  OMX_ErrorBadParameter = 0x80001005,

  /** The requested function is not implemented */
  OMX_ErrorNotImplemented = 0x80001006,

  /** The buffer was emptied before the next buffer was ready */
  OMX_ErrorUnderflow = 0x80001007,

  /** The buffer was not available when it was needed */
  OMX_ErrorOverflow = 0x80001008,

  /** The hardware failed to respond as expected */
  OMX_ErrorHardware = 0x80001009,

  /** The component is in the state OMX_StateInvalid */
  OMX_ErrorInvalidState = 0x8000100A,


  /** Stream is found to be corrupt */
  OMX_ErrorStreamCorrupt = 0x8000100B,

  /** Ports being connected are not compatible */
  OMX_ErrorPortsNotCompatible = 0x8000100C,

  /** Resources allocated to an idle component have been
      lost resulting in the component returning to the loaded state */
  OMX_ErrorResourcesLost = 0x8000100D,

  /** No more indicies can be enumerated */
  OMX_ErrorNoMore = 0x8000100E,

  /** The component detected a version mismatch */
  OMX_ErrorVersionMismatch = 0x8000100F,

  /** The component is not ready to return data at this time */
  OMX_ErrorNotReady = 0x80001010,

  /** There was a timeout that occurred */
  OMX_ErrorTimeout = 0x80001011,

  /** This error occurs when trying to transition into the state you are already in */
  OMX_ErrorSameState = 0x80001012,

  /** Resources allocated to an executing or paused component have been
      preempted, causing the component to return to the idle state */
  OMX_ErrorResourcesPreempted = 0x80001013,

  /** A non-supplier port sends this error to the IL client (via the EventHandler callback)
      during the allocation of buffers (on a transition from the LOADED to the IDLE state or
      on a port restart) when it deems that it has waited an unusually long time for the supplier
      to send it an allocated buffer via a UseBuffer call. */
  OMX_ErrorPortUnresponsiveDuringAllocation = 0x80001014,

  /** A non-supplier port sends this error to the IL client (via the EventHandler callback)
      during the deallocation of buffers (on a transition from the IDLE to LOADED state or
      on a port stop) when it deems that it has waited an unusually long time for the supplier
      to request the deallocation of a buffer header via a FreeBuffer call. */
  OMX_ErrorPortUnresponsiveDuringDeallocation = 0x80001015,


  /** A supplier port sends this error to the IL client (via the EventHandler callback)
      during the stopping of a port (either on a transition from the IDLE to LOADED
      state or a port stop) when it deems that it has waited an unusually long time for
      the non-supplier to return a buffer via an EmptyThisBuffer or FillThisBuffer call. */
  OMX_ErrorPortUnresponsiveDuringStop = 0x80001016,

  /** Attempting a state transtion that is not allowed */
  OMX_ErrorIncorrectStateTransition = 0x80001017,

  /* Attempting a command that is not allowed during the present state. */
  OMX_ErrorIncorrectStateOperation = 0x80001018,

  /** The values encapsulated in the parameter or config structure are not supported. */
  OMX_ErrorUnsupportedSetting = 0x80001019,

  /** The parameter or config indicated by the given index is not supported. */
  OMX_ErrorUnsupportedIndex = 0x8000101A,

  /** The port index supplied is incorrect. */
  OMX_ErrorBadPortIndex = 0x8000101B,

  /** The port has lost one or more of its buffers and it thus unpopulated. */
  OMX_ErrorPortUnpopulated = 0x8000101C,

  OMX_ErrorMax = 0x7FFFFFFF,
} OMX_ERRORTYPE;
#endif
