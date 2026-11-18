[English](USER_GUIDE.md) | Português (Brasil)

# Guia do haumea-server

O haumea-server mantém uma sessão multiplayer do [Haumea Studio](https://github.com/Haumea-Team/Haumea-Studio)
rodando numa máquina sempre ligada, para ninguém precisar deixar o Haumea aberto hospedando. As
pessoas entram nele pelo Haumea e constroem mapas juntas. Ele não precisa do jogo nem de tela.

- [Download](#download)
- [Primeira execução](#primeira-execução)
- [Entrando pelo Haumea](#entrando-pelo-haumea)
- [Contas, convites e papéis](#contas-convites-e-papéis)
- [Salas](#salas)
- [Levando o mapa para o seu servidor de jogo](#levando-o-mapa-para-o-seu-servidor-de-jogo)
- [Configurações](#configurações)
- [Rodando como serviço no Linux](#rodando-como-serviço-no-linux)
- [Windows](#windows)
- [Atualizando](#atualizando)
- [Backups](#backups)
- [O log](#o-log)
- [Problemas comuns](#problemas-comuns)

## Download

Baixe a versão mais recente em
[github.com/Haumea-Team/Haumea-Server/releases](https://github.com/Haumea-Team/Haumea-Server/releases):

| Arquivo | |
|---|---|
| `haumea-server-linux-x86_64` | o servidor para qualquer Linux 64 bits (estático: não precisa instalar mais nada) |
| `haumea-server-windows-x86_64.exe` | o servidor para Windows 64 bits |
| `install.sh`, `haumea-session.service`, `env.example` | para rodá-lo como serviço no Linux |
| `SHA256SUMS` | checksums: `sha256sum -c SHA256SUMS --ignore-missing` |

O servidor e o Haumea precisam falar o mesmo protocolo multiplayer. Quando não falam, o Haumea diz
qual lado está desatualizado; atualize esse.

## Primeira execução

No Linux:

```sh
mv haumea-server-linux-x86_64 haumea-server
chmod +x haumea-server
./haumea-server --state-dir state invite --role owner   # mostra o seu convite de dono
./haumea-server --state-dir state                        # roda o servidor
```

Tudo o que ele guarda (contas, convites, salas, a chave de identidade e o log) fica na pasta
`state`. Guarde essa pasta: ela é o seu servidor.

O servidor escuta na porta TCP **7790**. Abra-a no seu firewall (e no painel de firewall da sua
hospedagem, se tiver) para as pessoas conseguirem chegar nele.

Para um servidor que inicia junto com a máquina e se reinicia sozinho, veja
[Rodando como serviço no Linux](#rodando-como-serviço-no-linux).

## Entrando pelo Haumea

No Haumea, abra **Arquivo → Multiplayer...** e escolha **Entrar**:

1. Digite o endereço e a porta (7790) do servidor.
2. Na primeira vez, escolha **Código de convite**, cole o código e escolha um nome e uma senha. Isso
   cria a sua conta; o código é consumido.
3. Depois disso, escolha **Conta** e entre com o seu nome e senha.
4. Deixe **Sala** vazia para ver a lista de salas, ou digite uma. Como dono, você pode digitar um
   nome novo (`main`, por exemplo): a sala é criada e o mapa que você tem aberto vira o dela.

Na primeira vez que você entra, o Haumea mostra a **impressão digital** do servidor e a lembra.
Confira se ela bate com a que o servidor mostra (`haumea-server --state-dir state fingerprint`). Se
outro servidor responder naquele endereço algum dia, o Haumea se recusa a entrar: haveria algo no
meio.

## Contas, convites e papéis

As pessoas entram com um convite. Crie um no servidor:

```sh
./haumea-server --state-dir state invite                         # construtor, um uso, 7 dias
./haumea-server --state-dir state invite --role viewer --uses 5 --days 30
```

ou no Haumea, como administrador: **Administração → Criar um código de convite** no painel
Multiplayer.

| Papel | Pode |
|---|---|
| **viewer** (observador) | olhar e conversar; as edições dele são desfeitas |
| **builder** (construtor) | editar o mapa |
| **admin** (administrador) | também convidar, mudar papéis, remover pessoas, criar, trancar e apagar salas |
| **owner** (dono) | tudo, inclusive criar outros donos |

Um papel vale para o servidor todo ou só para uma sala. Administradores mudam papéis e removem
pessoas pelo Haumea (botão direito em alguém em **Pessoas**) ou no servidor:

| Comando | |
|---|---|
| `users` | as contas e os papéis delas |
| `role NOME P` | dá um papel a uma conta em todo lugar (`viewer`, `builder`, `admin`, `owner`) |
| `passwd NOME` | define a senha de uma conta (alguém esqueceu a sua) |
| `remove NOME` | apaga uma conta |
| `fingerprint` | a identidade deste servidor, como o Haumea a mostra |

Rode-os como `haumea-server --state-dir state COMANDO`. Eles funcionam com o servidor rodando: ele
aplica a mudança na hora.

Cinco senhas erradas de um mesmo endereço fazem esse endereço esperar dez minutos.

Qualquer pessoa numa sessão pode enviar modelos customizados do 0.3.DL, que o Haumea de todos
carrega. Convide pessoas em quem você confia e dê o papel de construtor com cuidado.

## Salas

Um servidor guarda vários mapas, cada um na sua sala, com as próprias pessoas, chat e saves.
Administradores:

- **criam** uma sala entrando com um nome de sala novo: o mapa que têm aberto vira o dela;
- **trancam** uma sala com uma senha, que todos os outros passam a precisar para entrar;
- **apagam** uma sala;
- decidem o que agentes de IA (o Namaka do Haumea) podem fazer em cada sala.

As pessoas trocam de sala, ou voltam ao lobby, pelo painel Multiplayer sem reconectar.

Cada sala é gravada em disco conforme muda, então um travamento ou reinício não perde nada.

## Levando o mapa para o seu servidor de jogo

Com `--maps-dir`, o servidor guarda o mapa de cada sala em Pawn:

```sh
./haumea-server --state-dir state --maps-dir maps \
    --map-out /home/samp/server/gamemodes/map.pwn --map-out-room main
```

- Sempre que alguém salva no Haumea, o servidor grava o mapa em `maps/<sala>/<nome>.pwn`, uma cópia
  datada em `maps/<sala>/history`, e avisa no chat.
- A cada 10 minutos em que o mapa mudou, ele pede o mapa a um editor da sala, então os arquivos ficam
  atualizados mesmo quando ninguém salva (`--autosave MIN` muda isso; `--autosave 0` desliga).
- `--map-out ARQUIVO` também grava o mapa de uma sala (`--map-out-room`, `main` por padrão) direto na
  pasta do seu servidor de jogo. Rode o haumea-server na mesma máquina do seu servidor open.mp ou
  SA-MP e o mapa estará lá quando você reiniciar o gamemode.

## Configurações

As configurações vêm dos padrões embutidos, depois de um arquivo `.env` (o da pasta de trabalho, ou
`--env ARQUIVO`), depois das variáveis de ambiente `HAUMEA_SERVER_*`, depois da linha de comando,
cada uma sobrepondo a anterior. O `env.example` lista todas, e o `haumea-server --help` também.

| Opção | Variável | |
|---|---|---|
| `--state-dir PASTA` | `HAUMEA_SERVER_STATE_DIR` | contas, convites, papéis e salas (salas em `PASTA/rooms`) |
| `--port N` | `HAUMEA_SERVER_PORT` | porta TCP (padrão 7790) |
| `--bind ENDEREÇO` | `HAUMEA_SERVER_BIND` | escuta só num endereço, IPv4 ou IPv6 (padrão: todos) |
| `--name TEXTO` | `HAUMEA_SERVER_NAME` | o nome do servidor no Haumea |
| `--max-clients N` | `HAUMEA_SERVER_MAX_CLIENTS` | editores ao mesmo tempo (padrão 32) |
| `--maps-dir PASTA` | `HAUMEA_SERVER_MAPS_DIR` | guarda o mapa de cada sala em Pawn |
| `--map-out ARQUIVO` | `HAUMEA_SERVER_MAP_OUT` | também grava o mapa de uma sala em ARQUIVO |
| `--map-out-room S` | `HAUMEA_SERVER_MAP_OUT_ROOM` | a sala do `--map-out` (padrão `main`) |
| `--autosave MIN` | `HAUMEA_SERVER_AUTOSAVE` | pede o mapa a cada MIN minutos em que ele mudou (padrão 10; 0: desligado) |
| `--log-file ARQUIVO` | `HAUMEA_SERVER_LOG_FILE` | o log (padrão `PASTA/log.txt`; vazio: nenhum) |
| `--no-log-chat` | `HAUMEA_SERVER_LOG_CHAT=0` | deixa o chat fora do log |
| `--no-log-connections` | `HAUMEA_SERVER_LOG_CONNECTIONS=0` | deixa quem entra e sai fora do log |
| `--timestamp-format F` | `HAUMEA_SERVER_TIMESTAMP_FORMAT` | formato strftime dos horários do log |
| `--no-update-check` | `HAUMEA_SERVER_UPDATE_CHECK=0` | não procura versão nova no GitHub |

### Uma sessão, sem contas

Sem `--state-dir`, o servidor roda uma sessão com uma senha, como o **Hospedar este mapa** do
Haumea:

```sh
./haumea-server --password segredo --state crew.hmps
```

Sem `--password` ele cria uma senha aleatória e a mostra. `--state ARQUIVO` mantém a sessão entre
reinícios. `--admin-password TEXTO` deixa só quem a informar carregar outro mapa para todos;
`--no-replace` não deixa ninguém.

## Rodando como serviço no Linux

O `install.sh` configura o haumea-server como um serviço do systemd. Coloque os arquivos da versão
numa pasta e rode:

```sh
mv haumea-server-linux-x86_64 haumea-server
chmod +x haumea-server
sudo sh install.sh
```

Ele:

- cria um usuário de sistema `haumea-session` e guarda tudo em `/var/lib/haumea-session`;
- instala o programa como `/usr/local/bin/haumea-session` e o serviço como `haumea-session`;
- lê as configurações de `/etc/haumea-session/env` (veja o `env.example`); as que você já tem ficam;
- abre a porta TCP 7790 no ufw quando o ufw está ativo (senão, abra-a no firewall da sua hospedagem);
- na primeira vez, mostra o seu convite de dono e a impressão digital do servidor.

Depois:

```sh
sudo systemctl status haumea-session
sudo systemctl restart haumea-session      # depois de mudar /etc/haumea-session/env
sudo journalctl -u haumea-session -f
```

Comandos sobre o estado do serviço rodam como o usuário dele:

```sh
sudo runuser -u haumea-session -- haumea-session --state-dir /var/lib/haumea-session invite
```

## Windows

O `haumea-server-windows-x86_64.exe` aceita as mesmas opções e comandos. Num terminal (PowerShell
ou cmd), na pasta onde você o baixou:

```bat
haumea-server-windows-x86_64.exe --state-dir state invite --role owner
haumea-server-windows-x86_64.exe --state-dir state
```

Permita-o no Firewall do Windows quando ele perguntar, e redirecione a porta TCP 7790 no roteador
se as pessoas forem entrar pela internet. Ele roda enquanto aquela janela estiver aberta; para
mantê-lo sozinho, rode-o pelo Agendador de Tarefas na inicialização ou como serviço com uma
ferramenta como o NSSM.

## Atualizando

Uma vez por dia o servidor pergunta à API do GitHub qual é a versão mais recente e registra um aviso
no log quando há uma mais nova (`--no-update-check` desliga isso). O Haumea também avisa quem tenta
entrar quando é o servidor que está desatualizado.

Para atualizar, pare o servidor, troque o programa e inicie-o de novo. A pasta de estado fica como
está. Com o serviço, rode o `install.sh` de novo com a versão nova: ele só atualiza o programa.

## Backups

Faça backup da pasta de estado (`/var/lib/haumea-session` no serviço), e da pasta de mapas se ela
estiver em outro lugar. Ela guarda:

| | |
|---|---|
| `accounts.txt`, `invites.txt` | contas (sem senhas, só chaves derivadas com Argon2id) e convites abertos |
| `identity.key` | a identidade do servidor: perca-a e o Haumea de todos verá um servidor diferente |
| `secret.key` | impede que se descubra quais nomes de conta existem |
| `rooms/` | o mapa, o histórico e as configurações de cada sala (trava, papéis, o que agentes podem fazer) |
| `log.txt` | o log |

Mantenha as chaves em segredo: quem tiver a `identity.key` pode se passar pelo seu servidor.

## O log

O `log.txt` é escrito como o do open.mp: `[2026-11-08T01:30:12+0000] [Info] ...`, com linhas
`[Warning]` e `[Error]` também. Ele tem quem entra e de onde, o chat e cada ação de administração
(convites, papéis, remoções, travas).

## Problemas comuns

| Problema | O que fazer |
|---|---|
| O Haumea não conecta | Confira se o servidor está rodando, se o endereço e a porta estão certos e se a porta TCP 7790 está aberta em todos os firewalls no caminho (o da máquina, o da hospedagem, o do roteador). |
| O Haumea diz que o servidor está desatualizado | Atualize o haumea-server. |
| O Haumea diz que ele está desatualizado | Atualize o Haumea (ele se atualiza sozinho, ou baixe-o). |
| O Haumea se recusa a entrar por causa da impressão digital | Outro servidor responde naquele endereço. Se você mudou ou reinstalou o servidor sem a `identity.key`, é esperado: esqueça o servidor salvo no Haumea e entre de novo. Caso contrário, não entre. |
| Senha errada, depois uma espera | Cinco senhas erradas fazem o endereço esperar dez minutos. Um administrador pode definir uma senha nova com `passwd NOME`. |
| As edições de um observador somem | É o papel de observador. Dê a ele `builder`. |
| O arquivo do mapa não atualiza | Ele atualiza ao salvar e a cada `--autosave` minutos em que o mapa mudou, e só com `--maps-dir`. Confira o log. |

Ainda com problema? Pergunte no [Discord do Haumea](https://discord.gg/YSxtANrsB4) ou abra uma issue.
