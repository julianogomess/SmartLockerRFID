# SmartLocker com leitor RFID e monitoramento Web.
### Juliano Gomes e Felipe Martinez

Projeto de uma fechadura eletrônica com leitura RFID e integração MQTT.

O projeto monitora e aciona uma fechadura elétrica. O monitoramento é feito via dashboard do Node-RED com auxilio do MQTT, assim consta o estado da porta, o estado da fechadura e o que o leitor rfid esta esperando. A ação de desbloquear a fechadura pode ser feita de duas formas, uma através de botão no dashboard ou através de cartão NFC autorizado.
O cadastro de cartões é autorizado pelo dashboard.
