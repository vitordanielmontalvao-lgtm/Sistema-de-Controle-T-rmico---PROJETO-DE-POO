#include <iostream>
#include <string>
#include <algorithm>
#include <locale>

using namespace std;

constexpr float TEMP_MIN = -40.0;
constexpr float TEMP_MAX = 125.0;
constexpr int   MARGEM_MIN = 30;
constexpr int   MARGEM_MAX = 70;
constexpr float FATOR_ALERTA = 0.5;
constexpr float FATOR_PERIGO = 1.2;

constexpr int SEGURA = 0, ALERTA = 1, PERIGO = 2;

long mapc(float x, float inMin, float inMax, long outMin, long outMax) {
    if (inMax == inMin) return outMin;
    long v = (long)((x - inMin) * (outMax - outMin) / (inMax - inMin) + outMin);
    long menor = min(outMin, outMax);
    long maior = max(outMin, outMax);
    if (v < menor) return menor;
    if (v > maior) return maior;
    return v;
}

int classificarZona(float temp, int margem) {
    if (temp == margem) return SEGURA;
    if (temp < FATOR_PERIGO * margem) return ALERTA;
    return PERIGO;
}

class Dispositivo {
protected:
    string nome;
    int pino;

public:
    Dispositivo(string n, int p) : nome(n), pino(p) {}

    virtual ~Dispositivo() {}

    string getNome() const { return nome; }
    int getPino() const { return pino; }

    virtual void atualizar(float temp, int margem, int zona) = 0;
};

class Ventoinha : public Dispositivo {
private:
    int pwmVentoinha;
    int rotacao;

public:
    Ventoinha(string n, int p) : Dispositivo(n, p), pwmVentoinha(0), rotacao(0) {}

    void atualizar(float temp, int margem, int zona) override {
        if (temp <= margem) {
            pwmVentoinha = 0;
            rotacao = 0;
        }
        else {
            pwmVentoinha = mapc(temp, margem, TEMP_MAX, 80, 255);
            rotacao = mapc(temp, margem, TEMP_MAX, 600, 2500);
        }

        cout << endl << "[" << nome << " - Pino " << pino << "] " << endl;
        cout << "PWM Ventoinha (MOSFET): " << pwmVentoinha << "/255 " << endl;
        cout << "Rotação Ventoinha: " << rotacao << " /2500 RPM" << endl;
    }
};

class CelulaPeltier : public Dispositivo {
private:
    int pwmPeltier;

public:
    CelulaPeltier(string n, int p) : Dispositivo(n, p), pwmPeltier(0) {}

    void atualizar(float temp, int margem, int zona) override {
        if (temp >= margem) {
            pwmPeltier = 0;
        }
        else {
            pwmPeltier = mapc(temp, margem, TEMP_MIN, 80, 255);
        }

        cout << endl << "[" << nome << " - Pino " << pino << "]" << endl;
        cout << "PWM Célula Peltier (MOSFET): " << pwmPeltier << "/255" << endl;
    }
};

class Sinalizador : public Dispositivo {
private:
    string corLED;
    string statusBuzzer;

public:
    Sinalizador(string n, int p) : Dispositivo(n, p), corLED("Desligado"), statusBuzzer("Silêncio") {}

    void atualizar(float temp, int margem, int zona) override {
        if (zona == SEGURA) {
            int brilhoVerde = mapc(temp, TEMP_MIN, FATOR_ALERTA * margem, 255, 100);
            corLED = "VERDE (Intensidade PWM: " + to_string(brilhoVerde) + ")";
            statusBuzzer = "Desligado";
        }
        else if (zona == ALERTA) {
            long intervalo = mapc(temp, FATOR_ALERTA * margem, FATOR_PERIGO * margem, 500, 100);
            corLED = "AMARELO (HIGH fixo)";
            statusBuzzer = "Bipes espaçados (110 Hz a cada " + to_string(intervalo) + " ms)";
        }
        else {
            long intervalo = mapc(temp, FATOR_PERIGO * margem, TEMP_MAX, 200, 50);
            corLED = "VERMELHO (HIGH fixo)";
            statusBuzzer = "Bipes rápidos (114 Hz a cada " + to_string(intervalo) + " ms)";
        }

        cout << endl << "[" << nome << " - Pino " << pino << "] " << endl;
        cout << "LED RGB: " << corLED << endl;
        cout << "Buzzer: " << statusBuzzer << endl;
    }
};

class SistemaControleTermico {
private:
    float temperatura;
    int margem;
    bool ativo;
    Dispositivo* dispositivos[3];

public:
    SistemaControleTermico() : temperatura(25.0), margem(50), ativo(true) {
        cout << endl << "Dispositivo ligado" << endl;
        dispositivos[0] = new Ventoinha("Módulo de Refrigeração (MOSFET)", 5);
        dispositivos[1] = new Sinalizador("Painel de Alerta (LED/Buzzer)", 12);
        dispositivos[2] = new CelulaPeltier("Módulo de Aquecimento (MOSFET)", 10);
    }

    ~SistemaControleTermico() {
        for (int i = 0; i < 3; i++) {
            delete dispositivos[i];
        }
    }

    void setAtivo(bool a) {
        ativo = a;
        if (ativo == false) {
            cout << endl << "Desligando sistema..." << endl;
        }
    }

    void setTemperatura(float t) {
        if (t < TEMP_MIN) temperatura = TEMP_MIN;
        else if (t > TEMP_MAX) temperatura = TEMP_MAX;
        else temperatura = t;
    }

    void setMargem(int m) {
        if (m < MARGEM_MIN) margem = MARGEM_MIN;
        else if (m > MARGEM_MAX) margem = MARGEM_MAX;
        else margem = m;
    }

    void executarCiclo() {
        if (!ativo) return;

        int zona = classificarZona(temperatura, margem);
        string nomeZona = (zona == SEGURA) ? "SEGURA" : (zona == ALERTA ? "ALERTA" : "PERIGO");

        cout << endl << "================ DISPLAY OLED ================" << endl;
        cout << "Temperatura do sensor: " << temperatura << "°C" << endl;
        cout << "Temperatura alvo: " << margem << "°C" << endl;
        cout << "Status da Zona: " << nomeZona << endl;
        cout << "--------------------------------------------------" << endl;

        for (int i = 0; i < 3; i++) {
            dispositivos[i]->atualizar(temperatura, margem, zona);
        }
        cout << "==================================================\n" << endl;
    }
};

int main() {
setlocale(LC_ALL, "Portuguese");

    SistemaControleTermico sistema;

    float tempSensor;
    int tempAlvo;
    char continuar;

    cout << endl << "=== SISTEMA DE CONTROLE TÉRMICO ===" << endl;

    do {
        cout << endl << "Digite a temperatura lida pelo sensor (-40°C a 125°C): " << endl;
        cin >> tempSensor;

        cout << "Digite a temperatura alvo do potenciômetro (30°C a 70°C): " << endl;
        cin >> tempAlvo;

        sistema.setTemperatura(tempSensor);
        sistema.setMargem(tempAlvo);
        sistema.executarCiclo();

        cout << endl << "Deseja realizar uma nova leitura? " << endl;
        cout << "S/s - Sim" << endl;
        cout << "N/n - Não" << endl;
        cout << endl << "Digite a tecla de resposta: " << endl;
        cin >> continuar;

    } while (continuar == 's' || continuar == 'S');

    sistema.setAtivo(false);

    cout << endl << "Operação encerrada" << endl;
    return 0;
}
