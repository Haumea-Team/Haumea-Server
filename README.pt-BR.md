[English](README.md) | Português (Brasil)

# haumea-server

O servidor multiplayer do [Haumea Studio](https://github.com/Haumea-Team/Haumea-Studio), o editor
de mapas para open.mp e SA-MP. As pessoas entram nele pelo Haumea (**Arquivo → Multiplayer →
Entrar**) e constroem um mapa juntas: veem as edições umas das outras na hora, as câmeras e
seleções umas das outras, e conversam num chat. Agentes de IA (o Namaka do Haumea) também podem
entrar, marcados como agentes e com o nome de quem são.

É um programa pequeno e sem dependências: rode-o na mesma máquina do seu servidor de jogo.

**Baixe-o** em [Releases](https://github.com/Haumea-Team/Haumea-Server/releases) (Linux e
Windows) e leia o **[guia](docs/USER_GUIDE.pt-BR.md)** para configurá-lo, convidar pessoas, usar
salas, rodá-lo como serviço, atualizar e fazer backups.

## O que ele faz

- **Contas e convites.** Você cria um convite; quem o recebe escolhe um nome e uma senha no Haumea
  e, a partir daí, entra com eles. Convites se esgotam e expiram.
- **Papéis.** Dono, administrador, construtor e observador, no servidor todo ou por sala. O servidor
  os aplica: as edições de um observador voltam desfeitas, e ações de administração são recusadas a
  todos os outros.
- **Salas.** Um servidor guarda vários mapas, cada um com as próprias pessoas, chat e saves.
  Administradores criam, trancam (com uma senha de sala) e apagam salas, e decidem o que agentes de
  IA podem fazer em cada uma.
- **Ele guarda tudo.** Cada sala é gravada em disco conforme muda, então um travamento ou reinício
  não perde nada. Com `--maps-dir` ele também guarda o mapa em Pawn sempre que alguém salva (e uma
  cópia datada), salva sozinho enquanto o mapa muda, e o `--map-out` pode gravá-lo na pasta
  gamemodes do seu servidor.
- **Criptografado, e prova quem é.** Toda conexão é criptografada (X25519, XChaCha20-Poly1305, do
  Monocypher), com a chave ligada à senha por Argon2id, e o servidor assina cada handshake com a
  própria identidade Ed25519. O Haumea lembra a impressão digital de um servidor e se recusa a entrar
  se outro responder naquele endereço depois. Um nome que ninguém tem é recusado do mesmo jeito que
  uma senha errada, e cinco senhas erradas de um endereço o fazem esperar dez minutos.
- **Log como o do open.mp.** `log.txt` com linhas `[2026-11-08T01:30:12+0000] [Info] ...`: quem entra
  e de onde, o chat e cada ação de administração (convites, papéis, remoções, travas).
- **Ping.** Todos veem o tempo de ida e volta de todos até o servidor.
- **Avisa quando está atrasado.** O Haumea se atualiza sozinho; quando um editor mais novo que o
  servidor tenta entrar, ele é avisado de que o servidor precisa ser atualizado, e o log diz isso.
  Uma vez por dia o servidor também pergunta à API do GitHub (só isso, com curl) qual é a versão mais
  recente e registra um aviso quando há uma mais nova; `--no-update-check` (ou
  `HAUMEA_SERVER_UPDATE_CHECK=0`) desliga isso. `--version` diz qual é esta.

## Compilar

```sh
make            # build/haumea-server, estático, para qualquer Linux x86-64
make windows    # build/haumea-server.exe (MinGW-w64)
make check
```

## Rodar

```sh
haumea-server --state-dir ./state                      # contas e salas, guardadas em ./state
haumea-server --state-dir ./state invite --role owner  # o seu primeiro convite
```

Entre com esse convite no Haumea, escolha seu nome e senha e digite um nome de sala (`main`, por
exemplo): como dono, você a cria, e o mapa que você tem aberto vira o dela. Convide outras pessoas
pelo painel Multiplayer do Haumea (**Administração → Criar um código de convite**) ou com
`haumea-server invite`.

### Comandos

| Comando | |
|---|---|
| `invite [--role P] [--uses N] [--days N]` | um código para criar uma conta (construtor, um uso, 7 dias por padrão) |
| `users` | as contas e os papéis delas |
| `role NOME P` | dá um papel a uma conta em todo lugar (`viewer`, `builder`, `admin`, `owner`) |
| `passwd NOME` | define a senha de uma conta |
| `remove NOME` | apaga uma conta |
| `fingerprint` | a identidade deste servidor, como o Haumea a mostra |

Eles funcionam com o servidor rodando: ele aplica a mudança na hora.

### Configurações

Dos padrões embutidos, depois de um arquivo `.env` (o da pasta de trabalho, ou `--env ARQUIVO`),
depois das variáveis `HAUMEA_SERVER_*`, depois da linha de comando. O `deploy/env.example` lista
todas; o `haumea-server --help` também.

Sem `--state-dir` ele roda uma sessão com uma senha (`--password`, `--state`), como um editor
hospedando pelo Haumea.

## Instalar como serviço (systemd)

Copie o `build/haumea-server` e a pasta `deploy/` para o servidor e rode:

```sh
sudo ./deploy/install.sh
```

Ele cria um usuário e um serviço `haumea-session`, guarda tudo em `/var/lib/haumea-session`, lê
`/etc/haumea-session/env`, abre a porta TCP 7790 no ufw e mostra o convite do dono e a impressão
digital do servidor. Rodá-lo de novo atualiza o programa. Um servidor de uma sessão de antes vira a
sala `main`.

## O Haumea e este repositório

Este repositório é dono do núcleo multiplayer em `core/` (o protocolo, o relay, as contas). O
Haumea mantém uma cópia para o próprio **Hospedar este mapa** e para entrar em servidores:
`scripts/sync-to-haumea.sh /caminho/para/haumea` a atualiza. Os dois lados conferem o número do
protocolo (`MP_PROTOCOL` em `core/mpprotocol.h`, hoje 5) ao conectar, então versões diferentes são
recusadas, não mal interpretadas; lance os dois juntos.

## Contribuir

Relatos de bugs e ideias são bem-vindos como issues; veja o [CONTRIBUTING.md](CONTRIBUTING.md).
Problemas de segurança vão em particular, como diz o [SECURITY.md](SECURITY.md). Dúvidas: o
[Discord do Haumea](https://discord.gg/YSxtANrsB4).

## Licença

GPL-2.0-or-later, como o Haumea. O Monocypher (BSD-2-Clause ou CC0) e o miniLZO
(GPL-2.0-or-later) estão em `core/`, com as licenças deles.
