// dashboard em html+javascript:
// https://dontpad.com/trabalho-IoT

//                                        Jogo da cobrinha
// A pontuação e o nome do participante é enviada para a dashboard acima através do "myWriteAPIKey"_ 
// no qual é apresentado 12 registros, sendo o maior deles destacado.
// È enviado também de volta através do "myReadAPIKey" a maior pontuação e exibida no display da ESP32.
// obs: para o recorde ser atualizado, é necessário que a pagina do html esteja aberta. 
// deixei ao final do codigo, o codigo html+javascript comentado caso o da pagina do dontpad apresente erro.


#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <ThingSpeak.h>

const unsigned long INTERVALO = 1000;
unsigned long tempoAnterior = 0;
String pontuacao;
String nome;
bool nomeInserido = false;

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 tela(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define NOME_REDE  "Wokwi-GUEST"
#define SENHA      ""

#define MEU_NUMERO_CANAL 2572428
#define MINHA_CHAVE_ESCRITA  "R6EV3MSGTMAN99QA"
#define MINHA_CHAVE_LEITURA  "H2T7PYFVDDTGOXK1"

WiFiClient clienteWiFi;

const byte pinosBotoes[] = {5, 19, 17, 18};

typedef enum {
  INICIO,
  JOGANDO,
  FIM_DE_JOGO,
  AGUARDANDO_CONEXAO
} Estado;

typedef enum {
  ESQUERDA,
  CIMA,
  DIREITA,
  BAIXO
} Direcao;

#define TAMANHO_PECA_COBRA 3
#define MAX_COMPRIMENTO_COBRA 165
#define TAMANHO_MAPA_X 20
#define TAMANHO_MAPA_Y 20
#define TAMANHO_INICIAL_COBRA 5
#define ATRASO_MOVIMENTO_COBRA 30

Estado estadoJogo;
Direcao direcao;
Direcao novaDirecao;
int8_t cobra[MAX_COMPRIMENTO_COBRA][2];
uint8_t comprimentoCobra;
int8_t fruta[2];

void setup() {
  Serial.begin(115200);
  delay(100);

  Serial.print("Se conectando a: ");
  Serial.println(NOME_REDE);

  WiFi.begin(NOME_REDE, SENHA);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }

  Serial.println("\nConectado");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  ThingSpeak.begin(clienteWiFi);

  if (!tela.begin(SSD1306_SWITCHCAPVCC, 0x3D)) {
    Serial.println(F("Falha na inicialização do SSD1306"));
    for (;;);
  }

  for (byte i = 0; i < 4; i++) {
    pinMode(pinosBotoes[i], INPUT_PULLUP);
  }

  randomSeed(analogRead(A0));

  tela.clearDisplay();
  desenharPromptDigiteNome();
  tela.display();
}

void inicializarJogo() {
  estadoJogo = AGUARDANDO_CONEXAO;
  direcao = DIREITA;
  novaDirecao = DIREITA;
  reiniciarCobra();
  gerarFruta();
  tela.clearDisplay();
  desenharMapa();
  desenharPontuacao();
  desenharAguardandoConexao();
  tela.display();
}

void reiniciarCobra() {
  comprimentoCobra = TAMANHO_INICIAL_COBRA;
  for (int i = 0; i < comprimentoCobra; i++) {
    cobra[i][0] = TAMANHO_MAPA_X / 2 - i;
    cobra[i][1] = TAMANHO_MAPA_Y / 2;
  }
}

unsigned long ultimoTempoAtualizacao = 0;

void loop() {
  if (!nomeInserido) {
    if (Serial.available() > 0) {
      nome = Serial.readStringUntil('\n');
      nome.trim();
      nomeInserido = true;
      inicializarJogo();
    }
  } else {
    unsigned long tempoAtual = millis();
    if (tempoAtual - ultimoTempoAtualizacao >= 10) {
      ultimoTempoAtualizacao = tempoAtual;

      switch (estadoJogo) {
        case AGUARDANDO_CONEXAO:
          estadoJogo = INICIO;
          tela.clearDisplay();
          desenharMapa();
          desenharPontuacao();
          desenharPressioneParaIniciar();
          tela.display();
          break;

        case INICIO:
          if (botaoPressionado()) estadoJogo = JOGANDO;
          break;

        case JOGANDO: {
          static int tempoMovimento = 0;
          tempoMovimento++;
          lerDirecao();
          if (tempoMovimento >= ATRASO_MOVIMENTO_COBRA) {
            direcao = novaDirecao;
            tela.clearDisplay();
            if (moverCobra()) {
              estadoJogo = FIM_DE_JOGO;
              pontuacao = String(comprimentoCobra - TAMANHO_INICIAL_COBRA);
              nome = String(nome);
              enviarParaThingSpeak(pontuacao, nome); 
              desenharFimDeJogo();
              delay(1000);
            }
            desenharMapa();
            desenharPontuacao();
            tela.display();
            verificarFruta();
            tempoMovimento = 0;
          }
          break;
        }
        case FIM_DE_JOGO:
          if (botaoPressionado()) {
            delay(500);
            inicializarJogo();
            estadoJogo = INICIO;
          }
          break;
      }
    }
  }
}

void enviarParaThingSpeak(String pontuacao, String nome) {
  ThingSpeak.setField(1, pontuacao);
  ThingSpeak.setField(2, nome);
  int resposta = ThingSpeak.writeFields(MEU_NUMERO_CANAL, MINHA_CHAVE_ESCRITA);
  if (resposta == 200) {
    Serial.println("Canal do ThingSpeak atualizado com sucesso.");
  } else {
    Serial.println("Problema na atualização: " + String(resposta));
  }
}

bool botaoPressionado() {
  for (byte i = 0; i < 4; i++) {
    if (digitalRead(pinosBotoes[i]) == LOW) {
      return true;
    }
  }
  return false;
}

void lerDirecao() {
  for (byte i = 0; i < 4; i++) {
    if (digitalRead(pinosBotoes[i]) == LOW && i != ((int)direcao + 2) % 4) {
      novaDirecao = (Direcao)i;
      return;
    }
  }
}

bool moverCobra() {
  int8_t x = cobra[0][0];
  int8_t y = cobra[0][1];

  switch (direcao) {
    case ESQUERDA:  x -= 1; break;
    case CIMA:      y -= 1; break;
    case DIREITA:   x += 1; break;
    case BAIXO:     y += 1; break;
  }

  if (verificarColisao(x, y))
    return true;

  for (int i = comprimentoCobra - 1; i > 0; i--) {
    cobra[i][0] = cobra[i - 1][0];
    cobra[i][1] = cobra[i - 1][1];
  }

  cobra[0][0] = x;
  cobra[0][1] = y;
  return false;
}

void verificarFruta() {
  if (fruta[0] == cobra[0][0] && fruta[1] == cobra[0][1]) {
    if (comprimentoCobra < MAX_COMPRIMENTO_COBRA)
      comprimentoCobra++;
    gerarFruta();
  }
}

void gerarFruta() {
  bool conflito;
  do {
    conflito = false;
    fruta[0] = random(0, TAMANHO_MAPA_X);
    fruta[1] = random(0, TAMANHO_MAPA_Y);
    for (int i = 0; i < comprimentoCobra; i++) {
      if (fruta[0] == cobra[i][0] && fruta[1] == cobra[i][1]) {
        conflito = true;
        break;
      }
    }
  } while (conflito);
}

bool verificarColisao(int8_t x, int8_t y) {
  for (int i = 1; i < comprimentoCobra; i++) {
    if (x == cobra[i][0] && y == cobra[i][1]) return true;
  }
  return (x < 0 || y < 0 || x >= TAMANHO_MAPA_X || y >= TAMANHO_MAPA_Y);
}

void desenharMapa() {
  int deslocamentoMapaX = SCREEN_WIDTH - TAMANHO_PECA_COBRA * TAMANHO_MAPA_X - 2;
  int deslocamentoMapaY = 2;

  tela.drawRect(fruta[0] * TAMANHO_PECA_COBRA + deslocamentoMapaX, fruta[1] * TAMANHO_PECA_COBRA + deslocamentoMapaY, TAMANHO_PECA_COBRA, TAMANHO_PECA_COBRA, SSD1306_INVERSE);
  tela.drawRect(deslocamentoMapaX - 2, 0, TAMANHO_PECA_COBRA * TAMANHO_MAPA_X + 4, TAMANHO_PECA_COBRA * TAMANHO_MAPA_Y + 4, SSD1306_WHITE);
  for (int i = 0; i < comprimentoCobra; i++) {
    tela.fillRect(cobra[i][0] * TAMANHO_PECA_COBRA + deslocamentoMapaX, cobra[i][1] * TAMANHO_PECA_COBRA + deslocamentoMapaY, TAMANHO_PECA_COBRA, TAMANHO_PECA_COBRA, SSD1306_WHITE);
  }
}

void desenharPontuacao() {
  tela.setTextSize(1);
  tela.setTextColor(SSD1306_WHITE);
  tela.setCursor(2, 3);
  tela.print("Pontos:");
  tela.println(comprimentoCobra - TAMANHO_INICIAL_COBRA);
  tela.print("");
  tela.println(nome);
}

void desenharPressioneParaIniciar() {
  String recorde = ThingSpeak.readStringField(MEU_NUMERO_CANAL, 3, MINHA_CHAVE_LEITURA);
  tela.setTextSize(1);
  tela.setTextColor(SSD1306_WHITE);
  tela.setCursor(2, 20);
  tela.print("Aperte um\nbotao para\niniciar \n");
  tela.println("\nRecorde: " + recorde);
}

void desenharAguardandoConexao() {
  tela.setTextSize(1);
  tela.setTextColor(SSD1306_WHITE);
  tela.setCursor(2, 20);
  tela.print("Aguarde \n conexao...");
}

void desenharFimDeJogo() {
  tela.setTextSize(1);
  tela.setTextColor(SSD1306_WHITE);
  tela.setCursor(0, 15);
  tela.println("");
  tela.setCursor(2, 25);
  tela.println("GAMEOVER");
  tela.setCursor(1, 35);
  tela.println("Aperte \npara enviar\n placar");
}

void desenharPromptDigiteNome() {
  tela.setTextSize(1);
  tela.setTextColor(SSD1306_WHITE);
  tela.setCursor(0, 20);
  tela.println("Digite seu nome\n no terminal:");
}



//                                                HTML

/* <!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ThingSpeak Data Widget</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            display: flex;
            justify-content: center;
            align-items: center;
            flex-direction: column;
            height: 100vh;
            margin: 0;
            background-color: #f4f4f4;
        }
        table {
            border-collapse: collapse;
            width: 80%;
            margin: 20px 0;
        }
        table, th, td {
            border: 1px solid #ddd;
        }
        th, td {
            padding: 12px;
            text-align: left;
        }
        th {
            background-color: #4CAF50;
            color: white;
        }
        tr:nth-child(even) {
            background-color: #f2f2f2;
        }
        .recorde {
            margin-top: 20px;
            font-size: 1.5em;
            color: #333;
        }
    </style>
</head>
<body>

<h2>Pontuação</h2>
<table id="dataTable">
    <thead>
        <tr>
            <th>Nome</th>
            <th>Pontos</th>
        </tr>
    </thead>
    <tbody>
    </tbody>
</table>

<div class="recorde" id="recorde"></div>

<script>
    document.addEventListener('DOMContentLoaded', function() {
        const channelID = '2572428';
        const apiKey = 'H2T7PYFVDDTGOXK1';
        const writeApiKey = 'R6EV3MSGTMAN99QA';
        const numResults = 12; 

        let recorde = { nome: '', pontos: 0 }; 

        const fetchData = () => {
            fetch(`https://api.thingspeak.com/channels/${channelID}/feeds.json?api_key=${apiKey}&results=${numResults}`)
                .then(response => response.json())
                .then(data => {
                    const feeds = data.feeds;
                    const tableBody = document.querySelector('#dataTable tbody');
                    tableBody.innerHTML = ''; 

                    
                    feeds.sort((a, b) => new Date(b.created_at) - new Date(a.created_at));

                    feeds.forEach(feed => {
                        const row = document.createElement('tr');
                        const nameCell = document.createElement('td');
                        const scoreCell = document.createElement('td');
                        const nome = feed.field2;
                        const pontos = parseInt(feed.field1, 10); // Convert score to integer

                        nameCell.textContent = nome;
                        scoreCell.textContent = pontos;

                        row.appendChild(nameCell);
                        row.appendChild(scoreCell);
                        tableBody.appendChild(row);

                       
                        if (pontos > recorde.pontos) {
                            recorde = { nome, pontos };

                           
                            fetch(`https://api.thingspeak.com/update?api_key=${writeApiKey}&field3=${recorde.pontos}`)
                                .then(response => response.text())
                                .then(result => console.log('ThingSpeak update response:', result))
                                .catch(error => console.error('Error updating ThingSpeak:', error));
                        }
                    });

                    
                    const recordeDiv = document.querySelector('#recorde');
                    recordeDiv.textContent = `Recorde - ${recorde.nome} : ${recorde.pontos} pontos`;
                })
                .catch(error => console.error('Error fetching data:', error));
        };

       
        fetchData();

     
        setInterval(fetchData, 30000); // 30000 milliseconds = 30 seconds
    });
</script>

</body>
</html> */
