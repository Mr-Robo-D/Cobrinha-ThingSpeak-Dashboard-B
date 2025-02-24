						Jogo da cobrinha
Projeto do popular game "snake" rodando e um dispositivo ESP32, no qual se comunica a uma pagina HTML para fazer atualização de pontos e guardando o resultado na plataforma "ThingSpeak" para futuras consultas.



Dashboard para vizualização do ranking em html+javascript:
https://dontpad.com/trabalho-IoT

Link para a execução do projeto diretamente no simulador Wokwi:
https://wokwi.com/projects/421881867536015361



--------------------------------------------------------------------------------------------------------------------------------------------
                                       
                                       
A pontuação e o nome do participante é enviada para a dashboard acima através do "myWriteAPIKey"_ 
no qual é apresentado 12 registros, sendo o maior deles destacado.
È enviado também de volta através do "myReadAPIKey" a maior pontuação e exibida no display da ESP32.
obs: para o recorde ser atualizado, é necessário que a pagina do html esteja aberta. 
deixei ao final do codigo, o codigo html+javascript comentado caso o da pagina do dontpad apresente erro.
