# hardware/nothing

## Soong options

| Namespace | Variable | Description | Default |
| --------- | -------- | ----------- | ------- |
| NOTHING_BIOMETRICS_FINGERPRINT | RUN_32BIT | Opt to run service in 32-bit mode only | false |
| NOTHING_BIOMETRICS_FINGERPRINT | IMPL_VER | Fingerprint implementation version (V1: legacy HAL 2.1 blob, V2: updated HAL blob) | V1 |
| NOTHING_BIOMETRICS_FINGERPRINT | USE_LHBM | Drive LHBM through /proc/touchpanel/fod_mode in libudfpshandler.asteroids | false |
| NOTHING_BIOMETRICS_FINGERPRINT | UDFPS_HANDLER | UDFPS handler to build (Spacewar, Pong or asteroids) | |

## UDFPS handlers

`android.hardware.biometrics.fingerprint-service.nothing` loads `libudfpshandler.so`.
All handlers install under that name, so only the one selected through
`UDFPS_HANDLER` is enabled. Select it and ship the matching module:

```make
$(call soong_config_set,NOTHING_BIOMETRICS_FINGERPRINT,UDFPS_HANDLER,Spacewar)
PRODUCT_PACKAGES += libudfpshandler.Spacewar
```

| Module | Device |
| ------ | ------ |
| libudfpshandler.Spacewar | Phone (1) |
| libudfpshandler.Pong | Phone (2) |
| libudfpshandler.asteroids | Phone (3a), Phone (3a) Pro |
