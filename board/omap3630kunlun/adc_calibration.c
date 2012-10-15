#include <common.h>
#include <command.h>
#include <nand.h>
#include <watchdog.h>
#include <asm/io.h>
#include <asm/arch/sys_proto.h>

/* Use do_nand for fastboot's flash commands */
extern int do_nand(cmd_tbl_t * cmdtp, int flag, int argc, char *argv[]);
/* Use do_setenv and do_saveenv to permenantly save data */
extern int do_saveenv (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[]);
extern int do_setenv ( cmd_tbl_t *cmdtp, int flag, int argc, char *argv[]);
extern void ets_cmd_send (const char *fmt, ...);
/*if defined, console has been used to exchange command*/
//#define ETS_DEBUG

#if defined(ETS_DEBUG)
#define ETS_PROMPT "ETS CMD #"
#define ETS_CMD_END_CHAR "\r\n"
#else
#define ETS_CMD_END_CHAR "\r"
#endif

#define ETS_BUF_SIZE (256)
#define ETS_MAX_ARGS (16)
#define ETS_CMD_SIZE   (20)
char *ets_calib_env="adc_calibrate";
char *wifi_mac_env = "wifi_mac";

static char ets_buffer[ETS_BUF_SIZE];
static char *ets_cmd_argv[ETS_MAX_ARGS + 1];	/* NULL terminated	*/

struct ets_commad{
    char label[ETS_CMD_SIZE];
    char name[ETS_CMD_SIZE];
    int (*handle)(struct ets_commad *, int, char *[]);
};

int SYS_CbpPower(struct ets_commad *cmd, int nargs, char *argv[])
{
    extern void omap_cbp_power_on(void);
    extern void omap_cbp_power_off(void);
    int ret = 0;
    gpio_t *gpio5_base = (gpio_t *)OMAP34XX_GPIO5_BASE;
    unsigned int temp = 0;

    if(nargs != 1){
        ret = -1;
        goto end;
    }

    switch(*argv[0]){
        case '0':
            omap_cbp_power_off();
            break;

        case '1':
            omap_cbp_power_on();
            break;

        case '?':
            sr32((u32)&gpio5_base->oe, 1, 1, 1);   /* GPIO 129 INPUT*/
            temp = !!(__raw_readl((u32)&gpio5_base->datain) & GPIO1);
            ets_cmd_send("%s:%s=%s", cmd->label, cmd->name, temp ? "on":"off");
            ets_cmd_send("%s", ETS_CMD_END_CHAR);
            return ret;

        default:
            ret = -1;
    }
end:
    ets_cmd_send("%s:%s=%s", cmd->label, cmd->name, ret ? "err":"ok");
    ets_cmd_send("%s", ETS_CMD_END_CHAR);
    return ret;
}

int SYS_ApBoot(struct ets_commad *cmd, int nargs, char *argv[])
{
    extern void twl4030_restart(void);
    int ret = 0;

    if(nargs != 1){
        ret = -1;
        goto end;
    }

    switch(*argv[0]){
        case '0':
            break;

        case '1':
            twl4030_restart();
            break;

        default:
            ret = -1;
    }
end:
    ets_cmd_send("%s:%s=%s", cmd->label, cmd->name, ret ? "err":"ok");
    ets_cmd_send("%s", ETS_CMD_END_CHAR);

    return ret;
}

int CALIB_AuxAdc(struct ets_commad *cmd, int nargs, char *argv[])
{
    extern int twl4030_battery_adc(void);
    int ret = 0;
    int adc = 0;

    if(nargs != 1){
        ret = -1;
        goto end;
    }

    switch(*argv[0]){
        case '?':
            adc = twl4030_battery_adc();
            if(adc < 0){
                ret = -1;
            }
            break;

        default:
            ret = -1;
    }

end:
    if(ret){
        ets_cmd_send("%s:%s=%s", cmd->label, cmd->name, "err");
    }else{
        ets_cmd_send("%s:%s=0x%x", cmd->label, cmd->name, adc);
    }

    ets_cmd_send("%s", ETS_CMD_END_CHAR);
    return ret;
}

/*[VBat ADC]*/
static int calib_table[ETS_MAX_ARGS][2];
int CALIB_WriteBatteryData(struct ets_commad *cmd, int nargs, char *argv[])
{
    int ret = 0;
    int i = 0;
    char *data = NULL;
    char *p0 = NULL, *p1 = NULL, *p2 = NULL;

    if(nargs < 1){
        ret = -1;
        goto end;
    }

    memset(calib_table, 0, sizeof(calib_table));

    for(i=0; i < nargs; i++){
        data = argv[i];
        p0 = strchr(data, '(');
        p1 = strchr(data, ',');
        p2 = strchr(data, ')');

        /*invalid data format*/
        if(!p0 || !p1 ||!p2 || (p0 > p1) || (p0 > p2) || (p1 > p2) ){
            ret = -1;
            memset(calib_table, 0, sizeof(calib_table));
            goto end;
        }

        /*get voltage*/
        p0++;
        calib_table[i][0] = simple_strtol(p0, NULL, 0);
        p1++;
        calib_table[i][1] = simple_strtol(p1, NULL, 0);

        p0 = p1 = p2 = NULL;
    }

end:
    ets_cmd_send("%s:%s=%s", cmd->label, cmd->name, ret ? "err":"ok");
    ets_cmd_send("%s", ETS_CMD_END_CHAR);
    return ret;
}

static char wifi_mac_buf[ETS_BUF_SIZE];
int CALIB_SetWifiMac(struct ets_commad *cmd, int nargs, char *argv[])
{
    int ret = 0;
#if 0
    int i = 0;
    char *data = NULL;
    char *p0 = NULL, *p1 = NULL, *p2 = NULL;
#endif

    if(nargs < 1){
        ret = -1;
        goto end;
    }

    memset(wifi_mac_buf, 0 , ETS_BUF_SIZE);
    memcpy(wifi_mac_buf, argv[0], strlen(argv[0]));

end:
    ets_cmd_send("%s:%s=%s", cmd->label, cmd->name, ret ? "err":"ok");
    ets_cmd_send("%s", ETS_CMD_END_CHAR);
    return ret;
}

static int ets_save_env(char *var, char *val)
{
    int ret = 0;
    char start[32], length[32];
    char ecc_type[32];
    char *lock[5]    = { "nand", "lock",   NULL, NULL, NULL, };
    char *unlock[5]  = { "nand", "unlock", NULL, NULL, NULL, };
    char *ecc[4]     = { "nand", "ecc",    NULL, NULL, };
    char *saveenv[2] = { "setenv", NULL, };
    char *setenv[4]  = { "setenv", NULL, NULL, NULL, };

    lock[2] = unlock[2] = start;
    lock[3] = unlock[3] = length;


    setenv[1] = var;
    setenv[2] = val;
    do_setenv(NULL, 0, 3, setenv);

    ecc[2] = ecc_type;
    sprintf(ecc_type, "hw");
    do_nand(NULL, 0, 3, ecc);

    sprintf(start, "0x%x", SMNAND_ENV_OFFSET);
    sprintf(length, "0x%x", SMNAND_ENV_LENGTH);


    /* This could be a problem is there is an outstanding lock */
    do_nand(NULL, 0, 4, unlock);
    ret = do_saveenv(NULL, 0, 1, saveenv);
    do_nand(NULL, 0, 4, lock);

    return ret;
}

int CALIB_Over(struct ets_commad *cmd, int nargs, char *argv[])
{
    int ret = 0;
    int i = 0, j = 0;
    int tmpvolt, tmpadc;
    char buf[ETS_BUF_SIZE];

    if(nargs > 0){
        ret = -1;
        goto end;
    }

    /* Sort the data by battery value decreasingly */
    for(i = 0; i < ETS_MAX_ARGS; i++){
        for(j = i + 1; j <ETS_MAX_ARGS; j++){
            if(calib_table[i][0] < calib_table[j][0]){
                tmpvolt = calib_table[i][0];
                tmpadc = calib_table[i][1];
                calib_table[i][0] = calib_table[j][0];
                calib_table[i][1] = calib_table[j][1];
                calib_table[j][0] = tmpvolt;
                calib_table[j][1] = tmpadc;
            }
        }
    }

    j = 0;
    memset(buf, 0, ETS_BUF_SIZE);

    for(i = 0; i < ETS_MAX_ARGS; i++){
        if(calib_table[i][0] > 0){
            j += sprintf(buf + j, "<%d,0x%x>,", calib_table[i][0], calib_table[i][1]);
        }
    }

    /*mark the last dot*/
    buf[j-1] = '\0';

#if defined(ETS_DEBUG)
    //ets_cmd_send("%s\n", buf);
#endif

    if(*buf){
        ets_save_env(ets_calib_env, buf);
    }
    if(*wifi_mac_buf){
        ets_save_env(wifi_mac_env, wifi_mac_buf);
    }
end:
    ets_cmd_send("%s:%s=%s", cmd->label, cmd->name, ret ? "err":"ok");
    ets_cmd_send("%s", ETS_CMD_END_CHAR);
    return ret;
}


#define ETS_CMD(l, n) { .label = #l, .name = #n, .handle = l ## _ ## n }

struct ets_commad ets_cmd_list[] = {
    ETS_CMD(SYS, CbpPower),
    ETS_CMD(SYS, ApBoot),
    ETS_CMD(CALIB, AuxAdc),
    ETS_CMD(CALIB, WriteBatteryData),
    ETS_CMD(CALIB, SetWifiMac),
    ETS_CMD(CALIB, Over),
    {{0}, {0}, NULL}
};

#if defined(ETS_DEBUG)
static char   erase_seq[] = "\b \b";		/* erase sequence	*/
static char   tab_seq[] = "        ";		/* used to expand TABs	*/
static char * ets_delete_char (char *buffer, char *p, int *colp, int *np, int plen)
{
    char *s;

    if (*np == 0) {
        return (p);
    }

    if (*(--p) == '\t') {			/* will retype the whole line	*/
        while (*colp > plen) {
            ets_cmd_send (erase_seq);
            (*colp)--;
        }
        for (s=buffer; s<p; ++s) {
            if (*s == '\t') {
                ets_cmd_send (tab_seq+((*colp) & 07));
                *colp += 8 - ((*colp) & 07);
            } else {
                ++(*colp);
                ets_cmd_send ("%c",*s);
            }
        }
    } else {
        ets_cmd_send (erase_seq);
        (*colp)--;
    }
    (*np)--;
    return (p);
}
#endif /* ETS_DEBUG */

int ets_read_line (void)
{
    char    *p = ets_buffer;
    int     n = 0;				/* buffer index		*/
    char    c = 0;

#if 0
    int	plen = 0;			/* prompt length	*/
    int	col;				/* output column cnt	*/
#endif

#if defined(ETS_DEBUG)
    /* print prompt */
    plen = strlen (ETS_PROMPT);
    ets_cmd_send (ETS_PROMPT);
    col = plen;
#endif

    for(; ;){
        WATCHDOG_RESET();		/* Trigger watchdog, if needed */

        c = getc();

        /*
         * Special character handling
         */
        switch (c) {
            case '\r':				/* Enter		*/
            case '\n':
                *p = '\0';
#if defined(ETS_DEBUG)
                ets_cmd_send("\r\n");
#endif
                return (p - ets_buffer);

            case '\0':				/* nul			*/
                continue;

#if defined(ETS_DEBUG)
            case 0x03:				/* ^C - break		*/
                ets_buffer[0] = '\0';	/* discard input */
                return (-1);

            case 0x15:				/* ^U - erase line	*/
                while (col > plen) {
                    ets_cmd_send (erase_seq);
                    --col;
                }
                p = ets_buffer;
                n = 0;
                continue;

            case 0x17:				/* ^W - erase word 	*/
                p=ets_delete_char(ets_buffer, p, &col, &n, plen);
                while ((n > 0) && (*p != ' ')) {
                    p=ets_delete_char(ets_buffer, p, &col, &n, plen);
                }
                continue;

            case 0x08:				/* ^H  - backspace	*/
            case 0x7F:				/* DEL - backspace	*/
                p=ets_delete_char(ets_buffer, p, &col, &n, plen);
                continue;
#endif

            default:
                /*
                 * Must be a normal character then
                 */
                if (n < ETS_BUF_SIZE-2) {
#if defined(ETS_DEBUG)
                    if (c == '\t') {	/* expand TABs		*/
                        ets_cmd_send (tab_seq+(col&07));
                        col += 8 - (col&07);
                    } else {
                        ++col;		/* echo input		*/
                        ets_cmd_send ("%c",c);
                    }
#endif
                    *p++ = c;
                    ++n;
                }else{
                    /* Buffer full */
#if defined(ETS_DEBUG)
                    ets_cmd_send ("\a");
#endif
                    return 0;
                }
        }
    }

}

int ets_parse_argus(char *cmd, char *argv[])
{
    int nargs = 0;
    char *p = NULL;
    char *line = NULL;

    p = strchr(cmd, '=');
    if(!p){
        /*no argument*/
        return nargs;
    }

    p++;
    line = p;

    while (nargs < ETS_MAX_ARGS) {
        /* skip any white space */
        while ((*line == ' ') || (*line == '\t')) {
            ++line;
        }

        if (*line == '\0') {	/* end of line, no more args	*/
            argv[nargs] = NULL;
            return (nargs);
        }

        argv[nargs++] = line;	/* begin of argument string	*/

        /* find end of string */
        while (*line && (*line != ' ') && (*line != '\t')) {
            ++line;
        }

        if (*line == '\0') {	/* end of line, no more args	*/
            argv[nargs] = NULL;
            return (nargs);
        }

        *line++ = '\0';		/* terminate current arg	 */
    }

#if defined(ETS_DEBUG)
    ets_cmd_send ("** Too many args (max. %d) **\n", CFG_MAXARGS);
#endif

    return nargs;
}

struct ets_commad* ets_find_cmd(char *cmd)
{
    struct ets_commad *cmdtp;
    char *l=NULL, *n=NULL;
    char label[ETS_CMD_SIZE];
    char name[ETS_CMD_SIZE];
    int i = 0;
    int cmd_found = 0;

    l = strchr(cmd, ':');

    /*No label*/
    if(l == NULL){
        return NULL;
    }

    /*get label string*/
    memset(label, 0, ETS_CMD_SIZE);
    memcpy(label, cmd, l - cmd);

    l++;
    /*No command*/
    if(*l == '\0'){
        return NULL;
    }

    n = strchr(l, '=');
    /*get name string*/
    memset(name, 0, ETS_CMD_SIZE);
    if(n){
        /*parameters behind*/
        memcpy(name, l, n - l);
    }else{
        /*No parameters*/
        memcpy(name, l, strlen(l));
    }

    for(i = 0; ets_cmd_list[i].handle; i++){
        cmdtp = &ets_cmd_list[i];
        if( !strncmp(label, cmdtp->label, ETS_CMD_SIZE) &&
                !strncmp(name, cmdtp->name, ETS_CMD_SIZE) ){
            cmd_found = 1;
            break;
        }
    }

    if(!cmd_found){
        cmdtp = NULL;
    }

    return cmdtp;
}

int ets_sync_ap(void)
{
    int len = 0;
    int ret = -1;

    for(; ;){
        memset(ets_buffer, 0, ETS_BUF_SIZE);
        len = ets_read_line();
        if(len > 0){
            if(!strncmp(ets_buffer, "Loopback 0x01", sizeof(ets_buffer))){
                ets_cmd_send("Loopback 0x01");
                ets_cmd_send("%s", ETS_CMD_END_CHAR);
                ret = 0;
                break;
            }
#if defined(ETS_DEBUG)
            else if(!strncmp(ets_buffer, "Exit", sizeof(ets_buffer))){
                break;
            }
#endif
        }
    }

    return ret;
}

int ets_process(void)
{
    DECLARE_GLOBAL_DATA_PTR;
    int ret = 0;
    int len = 0;
    int nargs = 0;
    struct ets_commad *ets_cmd = NULL;

    /*make console silent*/
    gd->flags |= GD_FLG_SILENT;

    ret = ets_sync_ap();
    if(ret < 0){
        goto end;
    }

    for(; ;){
        /*clear the buffer before receiveing the ETS command*/
        memset(ets_cmd_argv, 0, sizeof(ets_cmd_argv));
        memset(ets_buffer, 0, ETS_BUF_SIZE);

        len = ets_read_line();
        if(len > 0){
            ets_cmd = ets_find_cmd(ets_buffer);
            if(!ets_cmd){
                ets_cmd_send("SYS:error command");
                ets_cmd_send("%s", ETS_CMD_END_CHAR);
                continue;
            }

            nargs = ets_parse_argus(ets_buffer, ets_cmd_argv);
            ret = ets_cmd->handle(ets_cmd, nargs, ets_cmd_argv);
            /*boot the kernel after SYS:ApBoot=0*/
            if( !ret && !strcmp(ets_cmd->label, "SYS") && !strcmp(ets_cmd->name, "ApBoot") ){
                break;
            }
        }
    }

end:
    /*make console active*/
    gd->flags &= ~(GD_FLG_SILENT);
    return ret;
}
