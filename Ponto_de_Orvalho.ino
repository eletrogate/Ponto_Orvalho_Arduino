/***************************************************
  Monitorador de Ponto de Orvalho com Arduino NANO
  Criado em 04 de Outubro de 2021 por Michel Galvão
 ****************************************************/

// Inclusão das Biblitecas
#include <DHT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Configuração DHT11
#define DHTPIN 5
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// Definição dos pinos
#define pinLEDVerde 2
#define pinLEDAmarelo 3
#define pinLEDVermelho 4

// Configuração Display LCD I2C
/* Para saber o endereço I2C do Display LCD, faça o upolad do código
    de exemplo i2c_scanner. Você pode acessar o código
    em: Arquivo -> Exemplos -> Wire ->  i2c_scanner. Faça o upload,
    já com o circuito previamente montado, e abra o Monitor Serial.
    Você verá a mensagem 'I2C device found at address 0x27!'
    indicando o endereço à que o Display LCD está endereçado.
*/
#define enderecoLCD 0x27
LiquidCrystal_I2C lcd(enderecoLCD, 16, 2);


// Endereçamento dos caracteres personalizados criados para o display LCD
#define Subindo 0
#define Descendo 1
#define Neutro 2

// Caractere 'Subindo'
byte _Subindo[] = {
  B00000,
  B00100,
  B01010,
  B10001,
  B00000,
  B00000,
  B00000,
  B00000
};

// Caractere 'Descendo'
byte _Descendo[] = {
  B00000,
  B00000,
  B00000,
  B00000,
  B10001,
  B01010,
  B00100,
  B00000
};

// Caractere 'Neutro'
byte _Neutro[] = {
  B00000,
  B00000,
  B00000,
  B11111,
  B11111,
  B00000,
  B00000,
  B00000
};

// Variáveis Globais
float temperature = 25.0;
float umidade = 70.0;
float pontoDeOrvalho;
float ultimaLeituraPontoDeOrvalho;
unsigned long time;
const int qtdIndices = 50; // Média Móvel de 50 índices
const int intervaloInicial = 7; // 7 segundos : intervalo para não atualizar a tendência inicialmente pr 7 segundos iniciais
bool autorizadoATestarTendencia = false;
const long intervaloDeTendencia = 3600; // Intervalo de 1 hora (3600 segundos) para controle de tendência


void setup() {
  /* setup()
       - Descrição
           É chamada quando um sketch inicia. Usa-se para inicializar variáveis, configurar o modo dos pinos(INPUT ou OUTPUT),
           inicializar bibliotecas, etc. Será executada apenas uma vez, após a placa ser alimentada ou acontecer um reset.
       - Referência
           https://www.arduino.cc/reference/pt/language/structure/sketch/setup/
  */

  // Configuração dos LED's como saída
  pinMode(pinLEDVerde, OUTPUT);
  pinMode(pinLEDAmarelo, OUTPUT);
  pinMode(pinLEDVermelho, OUTPUT);

  // Inicialização LCD
  lcd.init();
  lcd.clear();
  lcd.backlight();
  lcd.createChar(Subindo, _Subindo);
  lcd.createChar(Descendo, _Descendo);
  lcd.createChar(Neutro, _Neutro);

  // inicialização DHT
  dht.begin();

  // Splash screen LCD (Tela de abertura)
  lcd.setCursor(0, 0);
  lcd.print("--.--");
  lcd.write(223); // caractere ° (grau)
  lcd.print(" ");
  lcd.write(Neutro); // caractere personalizado de tendência neutra
  lcd.print("________");
  lcd.setCursor(0, 1);
  lcd.print("________________");
  delay(1000);

  lcd.clear();
}

void loop() {
  /* loop()
       - Descrição
           Função que repete-se consecutivamente enaqunto a placa estiver ligada, permitindo o seu programa 
           mudar e responder a essas mudanças. Usa-se para controlar ativamente uma placa Arduino.
       - Referência
           https://www.arduino.cc/reference/pt/language/structure/sketch/loop/
  */

  // Faz a tentativa de atualização das variáveis de temperatura e umidade
  atualizaDados();

  // Atribui à variável 'pontoDeOrvalho' o valor da média móvel calculada do valor de Ponto de Orvalho
  pontoDeOrvalho = pontoDeOrvalhoMedia(calculaPontoOrvalho(temperature, umidade));

  // millis() retorna o número de milissegundos passados desde que a placa Arduino começou a executar o programa atual;
  if (millis() >= intervaloInicial * 1000) { // Se o valor de millis() for maior que o tempo de intervaloInicial em milissegundos, ...
    autorizadoATestarTendencia = true; // atribui o valor true para a variável
  }

  String tendencia; // Cria variável local para armazenar a tendência atual do ponto de Orvalho
  if (autorizadoATestarTendencia) { // Se o valor da variável for true, ...
    /* Exibe ao usuário:
        - o ponto de Orvalho,
        - a tendência do valor do ponto de Orvalho( se está subindo, neutro ou descendo),
        - e a sensação do ponto de orvalho sobre o ser humano.
    */
    lcd.setCursor(0, 0);
    lcd.print(pontoDeOrvalho);
    lcd.write(223); // Imprime na tela LCD o caractere de endereço 223 (11011111) da memória do controlador do display LCD (HD44780)
    lcd.print(" ");
    tendencia = verificaTendencia(); // atribue a variável tendencia o valor retornado da função 'verificaTendencia()', o qual verifica a tendencia do valor do ponto de orvalho

    /* .indexOf()
        - Descrição
            Localiza um caractere ou String dentro de outra String.
        - Sintaxe
            minhaString.indexOf(string2)
        - Parâmetros
            -> minhaString: uma variável do tipo String
            -> string2: a string a ser procurada - char ou String
        - Retorna
            A posição da minhaString especificada dentro do objeto String, ou -1 se não for encontrada.
        - Referência
            https://www.arduino.cc/reference/pt/language/variables/data-types/string/functions/indexof/
    */
    if (tendencia.indexOf("Subindo") != -1) { // Se for encontrado a String "Subindo" dentro de tendencia, ...
      lcd.write(Subindo); // Imprime no display LCD o caractere personalizado correspondente
    } else if (tendencia.indexOf("Descendo") != -1) { // Se não, se for encontrado a String "Descendo" dentro de tendencia, ...
      lcd.write(Descendo); // Imprime no display LCD o caractere personalizado correspondente
    } else if (tendencia.indexOf("Neutro") != -1) { // Se não, se for encontrado a String "Neutro" dentro de tendencia, ...
      lcd.write(Neutro); // Imprime no display LCD o caractere personalizado correspondente
    }
    lcd.print(tendencia); //  Imprime no display LCD o conteúdo da String tendencia

    sensacaoHumanaPontoOrvalho(pontoDeOrvalho); // Executa a função responsável por exibir no display LCD a sensação do ponto de orvalho sobre o Ser Humano,
    //sendo necessário informar o pontoDeOrvalho como parâmetro para a função.
  } else { // se não, ...
    // Exibe ao usuário que o sistema está em carregamento
    lcd.setCursor(0, 0);
    lcd.print("Iniciando       ");
    lcd.setCursor(0, 1);
    lcd.print("Leitura Sensor  ");
    // Esquanto o sistema estiver em carregamento, atribui o valor de 'pontoDeOrvalho' para a variável 'ultimaLeituraPontoDeOrvalho'
    ultimaLeituraPontoDeOrvalho = pontoDeOrvalho; // atualiza o último valor da leitura do ponto de orvalho
  }

  controladorLEDS(); // chama a função responsável pelo controle dos LED's de indicação do nível do Ponto de Orvalho
}

float calculaPontoOrvalho(float temp, float umid) {
  /* calculaPontoOrvalho()
        - Descrição
            Função responsável para o cálculo do ponto de Orvalho.
        - Sintaxe
            calculaPontoOrvalho(float temp, float umid)
        - Parâmetros
            -> temp: temperatura em graus Celsius - float
            -> umid: umidade em % RH - float
        - Retorna
            O ponto de Orvalho em graus Celsius.
  */

  float calculo_pontoOrvalho; // variável local para retorno do valor calculado
  float b; // variáveis de constantes do cálculo do ponto de orvalho
  float c;
  if (temp >= 0 && temp <= 50.00) {
    // constantes utilizadas em material fornecido pela(o): Journal of Applied Meteorology and Climatology
    //  Buck, A. L. (1981), "New equations for computing vapor pressure and enhancement factor", J. Appl. Meteorol. 20: 1527–1532
    b = 17.368;
    c = 238.88;
  } else if (temp >= -40.00 && temp <= 0) {
    // constantes utilizadas em material fornecido pela(o): Journal of Applied Meteorology and Climatology
    //  Buck, A. L. (1981), "New equations for computing vapor pressure and enhancement factor", J. Appl. Meteorol. 20: 1527–1532
    b = 17.966;
    c = 247.15;
  } else {
    // constantes utilizadas em material fornecido pela(o): Sonntag (1990)
    // SHTxx Application Note Dew-point Calculation
    // http://irtfweb.ifa.hawaii.edu/~tcs3/tcs3/Misc/Dewpoint_Calculation_Humidity_Sensor_E.pdf
    b = 17.62;
    c = 243.12;
  }

  float funcao1 = (log(umid / 100) + (b * temp) / (c + temp)); // cálculo do Ponto de Orvalho parcial
  calculo_pontoOrvalho = (c * funcao1) / (b - funcao1); // cálculo do Ponto de Orvalho finalizado

  return calculo_pontoOrvalho; // retorna a chamada da função o ponto de orvalho calculado
}

void atualizaDados() {
  /* atualizaDados()
        - Descrição
            Função responsável para atualizar os dados do sensor DHT11.
        - Sintaxe
            atualizaDados()
        - Parâmetros
            NENHUM
        - Retorna
            SEM RETORNO
  */


  // método isnan() retorna true se a variável passada como parâmetro for um número inválido
  if (!isnan(dht.readTemperature())) { // Se a leitura de temperatura do DHT11 for um número válido, ...
    temperature = dht.readTemperature(); // atribue o valor da leitura de temperatura do sensor DHT11 para a variável global de armazenagem da temperatura
  }
  if (!isnan(dht.readHumidity())) { // Se a leitura de umidade do DHT11 for um número válido, ...
    umidade = 70.0; // atribue o valor da leitura de umidade do sensor DHT11 para a variável global de armazenagem da umidade
  }
}

String verificaTendencia() {
  /* verificaTendencia()
        - Descrição
            Função responsável para verificar qual a tendência que o ponto de orvalho atual está.
        - Sintaxe
            verificaTendencia()
        - Parâmetros
            NENHUM
        - Retorna
            -> "Descendo ": caso o valor do ponto de orvalho esteja caindo, ou
            -> "Subindo  ": caso o valor do ponto de orvalho esteja se elevando, ou
            -> "Neutro  ":  caso o valor do ponto de orvalho esteja fixo.
  */

  String retorno; // variável local para retornar para a chamada da função

  if (millis() - time > intervaloDeTendencia * 1000) { // Se millis() subtraido do tempo intermediário for maior do que o intervalo pré-definido em milissegundos, ...
    time = millis(); // atualiza a variável global que armazena o tempo intermediário
    ultimaLeituraPontoDeOrvalho = pontoDeOrvalho; // atualiza o último valor da leitura do ponto de orvalho
  }


  if (ultimaLeituraPontoDeOrvalho > pontoDeOrvalho) { // Se  o último valor da leitura do ponto de orvalho for maior que a leitura atual de ponto de orvalho, ...
    //valor está caindo
    retorno = "Descendo "; // atribui à variável 'retorno' o valor correspondente
  } else if (ultimaLeituraPontoDeOrvalho < pontoDeOrvalho) { // Se  o último valor da leitura do ponto de orvalho for menor que a leitura atual de ponto de orvalho, ...
    //valor está subindo
    retorno = "Subindo  "; // atribui à variável 'retorno' o valor correspondente
  }
  else if (ultimaLeituraPontoDeOrvalho == pontoDeOrvalho) { // Se  o último valor da leitura do ponto de orvalho for igual à leitura atual de ponto de orvalho, ...
    //valor está subindo
    retorno = "Neutro  "; // atribui à variável 'retorno' o valor correspondente
  }

  return retorno; // retorna o valor da variável 'retorno'
}

float pontoDeOrvalhoMedia(float medidaAtual) {
  /* pontoDeOrvalhoMedia()
      - Descrição
          Função responsável para calcular a média móvel do ponto de orvalho.
      - Sintaxe
          pontoDeOrvalhoMedia(float medidaAtual)
      - Parâmetros
          -> medidaAtual: valor do ponto de orvalho atual - float
      - Retorna
          A média móvel do ponto de orvalho calculada.

  */

  // O qualificador de variável static torna a variável persistida entre chamadas da função, preservando seu valor.
  // Mais sobre static: https://www.arduino.cc/reference/pt/language/variables/variable-scope-qualifiers/static/
  static int numbers[qtdIndices]; // variável para armazenar os valores lidos do ponto de orvalho, tendo como quantidade de índices um valor definido pelo programador (qtdIndices)

  //desloca os elementos do vetor de média móvel
  for (int i = 0; i < qtdIndices; i++) { // estrutura de repetição que controla o deslocamento dos valores do pntOrvalho lido para o índice anterior
    numbers[i] = numbers[i + 1]; // faz o deslocamento dos valores do array com índice i + 1 para o índice i
  }
  numbers[qtdIndices] = medidaAtual; // atribui o valor 'medidaAtual' ao último índice do array numbers
  long indicesTotais = 0; // variável para armazenar a soma total dos valores do array numbers
  for (int i = 0; i < qtdIndices; i++) { // estrutura de repetição que controla a somatória de todos os valores dos índices do array numbers
    indicesTotais += numbers[i]; // faz a somatória
  }
  return indicesTotais / qtdIndices; // retorna a média calculada: todos valores somados (indicesTotais) / quantidade de medições (qtdIndices)

}

void controladorLEDS() {
  /* controladorLEDS()
       - Descrição
           Função responsável para o controle dos LED's de indicação do ponto de Orvalho.
       - Sintaxe
           controladorLEDS()
       - Parâmetros
           NENHUM
       - Retorna
           SEM RETORNO

  */

  if (pontoDeOrvalho > 24) { // se o pontoDeOrvalho for maior que 24, ...
    // liga LED VERMELHO e apaga os demais
    digitalWrite(pinLEDVermelho, HIGH);
    digitalWrite(pinLEDAmarelo, LOW);
    digitalWrite(pinLEDVerde, LOW);
  } else if ((pontoDeOrvalho > 21 && pontoDeOrvalho <= 24) || pontoDeOrvalho < 10) { // Se não, se o pontoDeOrvalho for maior que 21 E menor/igual que 24 OU menor que 10, ...
    // liga LED AMARELO e apaga os demais
    digitalWrite(pinLEDVermelho, LOW);
    digitalWrite(pinLEDAmarelo, HIGH);
    digitalWrite(pinLEDVerde, LOW);
  } else if (pontoDeOrvalho >= 10 && pontoDeOrvalho <= 18) { // Se não, se o pontoDeOrvalho for maior/igual que 10 E menor/igual que 18, ...
    // liga LED VERDE e apaga os demais
    digitalWrite(pinLEDVermelho, LOW);
    digitalWrite(pinLEDAmarelo, LOW);
    digitalWrite(pinLEDVerde, HIGH);
  }
}

void sensacaoHumanaPontoOrvalho(float ponto) {
  /* sensacaoHumanaPontoOrvalho()
        - Descrição
            Função responsável para exibição da sensação Humana sobre o ponto de orvalho no display LCD.
        - Sintaxe
            sensacaoHumanaPontoOrvalho(float ponto)
        - Parâmetros
            -> ponto: Ponto de Orvalho atual - float
        - Retorna
            SEM RETORNO

  */
  if (ponto > 29) { // se o ponto de orvalho for maior que 29, ...
    lcd.setCursor(0, 1);
    lcd.print("Opressao Severa ");
  } else if (ponto > 26) { // Se não, se o ponto de orvalho for maior que 26, ...
    lcd.setCursor(0, 1);
    lcd.print("Mort. alg. doenc");
  } else if (ponto > 24) { // Se não, se o ponto de orvalho for maior que 24, ...
    lcd.setCursor(0, 1);
    lcd.print("Ext. Desconfor. ");
  } else if (ponto > 21) { // Se não, se o ponto de orvalho for maior que 21, ...
    lcd.setCursor(0, 1);
    lcd.print("Desconfortavel  ");
  } else if (ponto > 18) { // Se não, se o ponto de orvalho for maior que 18, ...
    lcd.setCursor(0, 1);
    lcd.print("Desconf. maioria");
  } else if (ponto > 16) { // Se não, se o ponto de orvalho for maior que 16, ...
    lcd.setCursor(0, 1);
    lcd.print("OK para maioria ");
  } else if (ponto > 13) { // Se não, se o ponto de orvalho for maior que 13, ...
    lcd.setCursor(0, 1);
    lcd.print("Confortavel     ");
  } else if (ponto > 10) { // Se não, se o ponto de orvalho for maior que 10, ...
    lcd.setCursor(0, 1);
    lcd.print("Muito Confortave");
  } else if (ponto <= 10) { // Se não, se o ponto de orvalho for menor/igual que 10, ...
    lcd.setCursor(0, 1);
    lcd.print("Seco p/ alguns  ");
  }
}