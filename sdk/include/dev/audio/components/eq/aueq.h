#ifndef _AUEQ_H
#define _AUEQ_H

#ifdef __cplusplus
extern "C" {
#endif


/**
  * @brief AUEQ ioctl_cmd type
  */
enum aueq_ioctl_cmd {

    /*!
     * @brief:
     * 
     */
    AUEQ_IOCTL_CMD_NONE,
};


/*!
 * @brief coefficient configuration
 * @note
 *     a example of coefficient for 5 band:
 *     struct aueq_cfg cfg[5];
 *     cfg[0].b[0] = 0.1;
 *     cfg[0].b[1] = 0.3;
 *     ...
 *     cfg[1].b[1] = 0.6;
 */
struct aueq_cfg{
  double b[3];
  double a[2];
};


/* AUEQ api for user */

struct aueq_device {
    struct dev_obj dev;
};

struct aueq_hal_ops {
    struct devobj_ops ops;
    int32(*open)(struct aueq_device *aueq, struct aueq_cfg *p_cfg);
    int32(*close)(struct aueq_device *aueq);
    int32(*run)(struct aueq_device *aueq, void* dat_buf, uint32 bytes);
    int32(*ioctl)(struct aueq_device *aueq, enum aueq_ioctl_cmd ioctl_cmd, uint32 param1, uint32 param2);
};



/* AUEQ API functions */



/**
 * @brief aueq_open
 *
 *
 * @note  .
 *
 * @param aueq       The address of aueq device in the RTOS.
 * @param .
 * @param .
 *
 * @return
 *     - RET_OK              Success
 *     - RET_ERR             Fail
 */
int32 aueq_open(struct aueq_device *aueq, struct aueq_cfg *p_cfg);

/**
 * @brief aueq_close
 *
 *
 * @note  .
 *
 * @param aueq       The address of aueq device in the RTOS.
 * @param .
 * @param .
 *
 * @return
 *     - RET_OK              Success
 *     - RET_ERR             Fail
 */
int32 aueq_close(struct aueq_device *aueq);

/**
 * @brief aueq_encode
 *
 *
 * @note  .
 *
 * @param aueq       The address of aueq device in the RTOS.
 * @param .
 * @param .
 *
 * @return
 *     - RET_OK              Success
 *     - RET_ERR             Fail
 */
int32 aueq_run(struct aueq_device *aueq, void* dat_buf, uint32 bytes);


/**
 * @brief aueq_ioctl
 *
 *
 * @note  .
 *
 * @param aueq       The address of aueq device in the RTOS.
 * @param .
 * @param .
 *
 * @return
 *     - RET_OK              Success
 *     - RET_ERR             Fail
 */
int32 aueq_ioctl(struct aueq_device *aueq, enum aueq_ioctl_cmd ioctl_cmd, uint32 param1, uint32 param2);

#ifdef __cplusplus
}
#endif
#endif
