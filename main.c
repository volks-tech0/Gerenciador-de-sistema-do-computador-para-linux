#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/statvfs.h>
#include <sys/sysinfo.h>
#include <sys/ioctl.h>

void statusGeral(void);
void usoCPU(void);
void usoMemoria(void);
void usoDisco(void);
void informacoesProcessos(void);
void diagnosticoGeral(void);

typedef struct
{
    unsigned long long user, nice, system, idle, iowait, irq, softirq, steal;
} CpuTimes;

int lerTemposCPU(CpuTimes *t)
{
    FILE *fp = fopen("/proc/stat", "r");
    if (!fp) return 0;

    char label[16];
    int lidos = fscanf(fp, "%15s %llu %llu %llu %llu %llu %llu %llu %llu",
                       label, &t->user, &t->nice, &t->system, &t->idle,
                       &t->iowait, &t->irq, &t->softirq, &t->steal);
    fclose(fp);
    return (lidos == 9);
}

double calcularUsoCPU(void)
{
    CpuTimes t1, t2;
    if (!lerTemposCPU(&t1)) return -1.0;
    usleep(300000);
    if (!lerTemposCPU(&t2)) return -1.0;

    unsigned long long idle1 = t1.idle + t1.iowait;
    unsigned long long idle2 = t2.idle + t2.iowait;
    unsigned long long total1 = t1.user + t1.nice + t1.system + t1.idle +
                                t1.iowait + t1.irq + t1.softirq + t1.steal;
    unsigned long long total2 = t2.user + t2.nice + t2.system + t2.idle +
                                t2.iowait + t2.irq + t2.softirq + t2.steal;
    unsigned long long totalDelta = total2 - total1;
    unsigned long long idleDelta = idle2 - idle1;

    if (totalDelta == 0) return 0.0;
    return (double)(totalDelta - idleDelta) * 100.0 / (double)totalDelta;
}

int lerMemoria(long *totalKB, long *disponivelKB)
{
    FILE *fp = fopen("/proc/meminfo", "r");
    if (!fp) return 0;

    char linha[256];
    int achouTotal = 0, achouDisp = 0;
    while (fgets(linha, sizeof(linha), fp))
    {
        if (sscanf(linha, "MemTotal: %ld kB", totalKB) == 1) achouTotal = 1;
        else if (sscanf(linha, "MemAvailable: %ld kB", disponivelKB) == 1) achouDisp = 1;
        if (achouTotal && achouDisp) break;
    }
    fclose(fp);
    return achouTotal && achouDisp;
}

double lerTemperatura(void)
{
    FILE *fp = fopen("/sys/class/thermal/thermal_zone0/temp", "r");
    if (!fp) return -1.0;

    long milicelsius;
    if (fscanf(fp, "%ld", &milicelsius) != 1)
    {
        fclose(fp);
        return -1.0;
    }
    fclose(fp);
    return milicelsius / 1000.0;
}

void formatarTempo(long segundos, char *buffer, size_t tamanho)
{
    long dias = segundos / 86400;
    long horas = (segundos % 86400) / 3600;
    long minutos = (segundos % 3600) / 60;
    snprintf(buffer, tamanho, "%ldd %ldh %ldm", dias, horas, minutos);
}

int main(void)
{
    int opcao;
    do
    {
        system("clear");
        printf("========================================\n");
        printf("        VERIFICADOR DO SISTEMA         \n");
        printf("========================================\n\n");
        printf("1 - Status geral\n");
        printf("2 - Uso de CPU\n");
        printf("3 - Uso de memoria\n");
        printf("4 - Uso de disco\n");
        printf("5 - Informacoes de processos\n");
        printf("6 - Diagnostico geral\n");
        printf("0 - Sair\n\n");
        printf("Escolha uma opcao: ");

        if (scanf("%d", &opcao) != 1)
        {
            opcao = -1;
            while (getchar() != '\n');
        }

        switch (opcao)
        {
            case 1: statusGeral(); break;
            case 2: usoCPU(); break;
            case 3: usoMemoria(); break;
            case 4: usoDisco(); break;
            case 5: informacoesProcessos(); break;
            case 6: diagnosticoGeral(); break;
            case 0: printf("\nEncerrando o verificador...\n"); break;
            case 67: system("clear");
                    sixseven();
                    break;
            default: printf("\nOpcao invalida!\n"); break;
        }

        if (opcao != 0)
        {
            printf("\nPressione ENTER para continuar...");
            getchar();
            getchar();
        }
    } while (opcao != 0);

    return 0;
}

void statusGeral(void)
{
    printf("\n========== STATUS GERAL ==========\n");
    struct sysinfo info;
    if (sysinfo(&info) == 0)
    {
        char tempoFormatado[64];
        formatarTempo(info.uptime, tempoFormatado, sizeof(tempoFormatado));
        printf("Tempo ligado: %s\n", tempoFormatado);
        printf("Processos ativos: %u\n", info.procs);
        printf("Carga media (1/5/15 min): %.2f / %.2f / %.2f\n",
               info.loads[0] / 65536.0, info.loads[1] / 65536.0, info.loads[2] / 65536.0);
    }
    else printf("Nao foi possivel obter informacoes do sistema.\n");

    double cpu = calcularUsoCPU();
    if (cpu >= 0) printf("Uso de CPU: %.1f%%\n", cpu);
    else printf("Uso de CPU: indisponivel\n");

    long totalKB, dispKB;
    if (lerMemoria(&totalKB, &dispKB))
        printf("Uso de memoria: %.1f%%\n", 100.0 * (totalKB - dispKB) / (double)totalKB);

    double temp = lerTemperatura();
    if (temp >= 0) printf("Temperatura: %.1f C\n", temp);
    else printf("Temperatura: sensor nao disponivel\n");
}

void usoCPU(void)
{
    printf("\n========== USO DE CPU ==========\nCalculando uso de CPU...\n");
    double cpu = calcularUsoCPU();
    if (cpu >= 0)
    {
        printf("Uso atual: %.1f%%\n", cpu);
        if (cpu < 30) printf("Nivel: BAIXO\n");
        else if (cpu < 70) printf("Nivel: MODERADO\n");
        else printf("Nivel: ALTO\n");
    }
    else printf("Nao foi possivel ler /proc/stat.\n");
}

void usoMemoria(void)
{
    printf("\n========== USO DE MEMORIA ==========\n");
    long totalKB, dispKB;
    if (lerMemoria(&totalKB, &dispKB))
    {
        long usadoKB = totalKB - dispKB;
        printf("Total:      %6ld MB\n", totalKB / 1024);
        printf("Em uso:     %6ld MB\n", usadoKB / 1024);
        printf("Disponivel: %6ld MB\n", dispKB / 1024);
        printf("Uso:        %.1f%%\n", 100.0 * usadoKB / (double)totalKB);
    }
    else printf("Nao foi possivel ler /proc/meminfo.\n");
}

void usoDisco(void)
{
    printf("\n========== USO DE DISCO ==========\n");
    struct statvfs st;
    if (statvfs("/", &st) == 0)
    {
        unsigned long long totalBytes = (unsigned long long)st.f_blocks * st.f_frsize;
        unsigned long long livreBytes = (unsigned long long)st.f_bfree * st.f_frsize;
        unsigned long long usadoBytes = totalBytes - livreBytes;
        printf("Particao: /\nTotal:  %.2f GB\nUsado:  %.2f GB\nLivre:  %.2f GB\nUso:    %.1f%%\n",
               totalBytes / (1024.0 * 1024.0 * 1024.0),
               usadoBytes / (1024.0 * 1024.0 * 1024.0),
               livreBytes / (1024.0 * 1024.0 * 1024.0),
               100.0 * usadoBytes / (double)totalBytes);
    }
    else printf("Nao foi possivel ler informacoes do disco.\n");
}

void informacoesProcessos(void)
{
    printf("\n===== INFORMACOES DE PROCESSOS =====\n");
    struct sysinfo info;
    if (sysinfo(&info) == 0) printf("Processos ativos: %u\n", info.procs);
    else printf("Nao foi possivel obter a contagem de processos.\n");
    printf("\n(Dica: para ver a lista detalhada, use 'top' ou 'ps aux' no terminal)\n");
}

void diagnosticoGeral(void)
{
    printf("\n========== DIAGNOSTICO GERAL ==========\n");
    int alertas = 0;
    double cpu = calcularUsoCPU();
    if (cpu >= 0)
    {
        printf("CPU............ %.1f%% -> %s\n", cpu, cpu > 85 ? "ALERTA" : "OK");
        if (cpu > 85) alertas++;
    }

    long totalKB, dispKB;
    if (lerMemoria(&totalKB, &dispKB))
    {
        double memPercent = 100.0 * (totalKB - dispKB) / (double)totalKB;
        printf("Memoria........ %.1f%% -> %s\n", memPercent, memPercent > 90 ? "ALERTA" : "OK");
        if (memPercent > 90) alertas++;
    }

    struct statvfs st;
    if (statvfs("/", &st) == 0)
    {
        unsigned long long totalBytes = (unsigned long long)st.f_blocks * st.f_frsize;
        unsigned long long livreBytes = (unsigned long long)st.f_bfree * st.f_frsize;
        double discoPercent = 100.0 * (totalBytes - livreBytes) / (double)totalBytes;
        printf("Disco.......... %.1f%% -> %s\n", discoPercent, discoPercent > 90 ? "ALERTA" : "OK");
        if (discoPercent > 90) alertas++;
    }

    double temp = lerTemperatura();
    if (temp >= 0)
    {
        printf("Temperatura.... %.1f C -> %s\n", temp, temp > 80 ? "ALERTA" : "OK");
        if (temp > 80) alertas++;
    }
    else printf("Temperatura.... indisponivel\n");

    printf("\nSTATUS GERAL: %s\n", alertas == 0 ? "NORMAL" : "ATENCAO NECESSARIA");
}
void sixseven(void)
{

    struct winsize terminal;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &terminal);

    int margem = (terminal.ws_col - 70) / 2;

    if (margem < 0)
        margem = 0;
    printf("\033[%dC⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣀⣀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣀⣀⣀⣀⣀⣀⣀⣀⣀⣀⣀⠀\n", margem);
    printf("\033[%dC⠀⠀⠀⠀⠀⢀⠤⠒⠈⠉⣠⣤⣤⣄⠈⠁⠒⢤⣤⣤⡀⠀      ⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡿\n", margem);
    printf("\033[%dC⠀⠀⣠⣶⣿⣿⣶⣎⠁⠀⠀⠀⠀⠻⠋⠁⠈⠀⠀⠀⠈⠉⠻⡇⠀⠀⠀⠀⠛⠛⠛⠛⠛⢻⣿⣿⣿⠟⠁\n", margem);
    printf("\033[%dC ⣼⣿⡟⠉⡹⡿⡿⠇⠀⠀⠀⠀⠀⠀⠀⠀⠀⢄⠀⠀⠀⠑⢜⣆⠀⠀⠀⠀⠀⠀⠀⣰⣿⣿⣿⠋⠀⠀\n", margem);
    printf("\033[%dC⢠⣿⣿⣧⣶⣷⣦⣄⠀⠀⠀⠀⠀⣀⣠⢤⠤⠤⠤⣵⣤⣤⠐⢒⡏⡄⠀⠀⠀⠀⠀⣼⣿⣿⣿⠃⠀⠀⠀\n", margem);
    printf("\033[%dC⢸⣿⣿⡟⡍⠙⣿⣿⡆⠀⠀⠀⠸⡁⢿⣿⠇⠀⠀⣼⠿⠿⠀⢠⡟⡇⠀⠀⠀⠀⣰⣿⣿⣿⡇⠀⠀⠀⠀\n", margem);
    printf("\033[%dC⠈⣿⣿⡆⣇⢀⣿⣿⡇⠀⠀⠀⠀⠑⠤⣀⡀⠤⠊⠀⠑⠂⠰⠟⠁⡇⠀⠀⠀⢀⣿⣿⣿⣿⠁⠀⠀⠀⠀\n", margem);
    printf("\033[%dC ⠙⢿⣿⣾⣿⣿⡟⠁⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢰⠁⠀⠀⢠⠁⡹⠋⠩⣭⣤⣤⡀⠀⠀\n", margem);
    printf("\033[%dC   ⠉⢹⣏⡉⢦⠀⠀⠀⠀⠀⠀⢤⠄⣀⣀⣀⣀⣀⣀⡀⢀⠆⠀⠀⠀⣂⠒⠂⢉⠐⠚⠚⢲⠇⠀⠀\n", margem);
    printf("\033[%dC⡔⠀⠏⠓⢷⣼⡿⡓⡦⡀⠀⠀⠀⠀⠀⣠⣀⣀⡀⠀⠀⢠⠎⠀⠀⠀⠀⠙⠛⠋⠉⠉⠉⠉⠁⠀⠀⠀\n", margem);
    printf("\033[%dC⠈⠈⠑⠒⠛⢇⣨⣿⣼⣃⡀⠀⠀⠀⠀⠀⠉⠀⠀⣀⠴⠁⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀\n", margem);
    printf("\033[%dC⠀⠀⠀⠀⠀⠀⠈⠙⠛⠷⠶⠶⠶⠶⠶⢒⣉⣁⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀\n", margem);
    printf("\033[%dC⠀⠀⠀⠀⠀⠀⢀⣀⣀⣀⣀⡀⠀⠀⠀⡟⢿⣿⣿⣷⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀\n", margem);
    printf("\033[%dC⠀⠀⠀⠀⠀⠀⢸⣿⣿⣿⣿⣿⠁⠀⠀⡇⠀⢻⡿⣿⣶⣤⣀⡀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀\n", margem);
    printf("\033[%dC⠀⠀⠀⠀⠀⠀⢸⠟⠈⡻⣿⡿⣶⠤⢼⣧⣴⡅⠉⠋⡁⣀⣌⣹⡄⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀\n", margem);
    printf("\033[%dC⠀⠀⠀⠀⠀⠀⣾⣄⡜⠀⠘⠋⠀⣠⣆⣨⣯⠓⠯⠩⠭⠷⠛⠊⠁⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀\n", margem);
    printf("\033[%dC⠀⠀⠀⠀⠀⠀⠈⠙⠺⠯⣛⣉⣭⠱⠤⠚⠉⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀\n", margem);
}
