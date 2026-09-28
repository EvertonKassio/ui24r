/*
 * state.h - Estado (local, em memoria) de mute de cada canal.
 *
 * POR ENQUANTO (sem acesso a mesa) tudo isso e so local: tocar em um
 * botao apenas muda a cor na tela. Quando a comunicacao com a Ui24R
 * for implementada, o lugar certo para "plugar" isso e a funcao
 * enviar_mute_para_mesa() no fim deste arquivo - ela ja e chamada
 * toda vez que o usuario muda o estado de um canal pela interface.
 */
#pragma once
#include <stdint.h>
#include "data.h"

/* true = canal ativado (som passa); false = mutado */
extern bool estado_canal[];

void estado_init();

bool canal_get_ativo(int indice_canal);
void canal_set_ativo(int indice_canal, bool ativo);
void canal_alternar(int indice_canal);

/* Um grupo aparece "ativado" (botao preenchido) se pelo menos um dos
 * canais dele estiver ativo. Tocar no bloco do grupo alterna todos os
 * canais do grupo juntos (liga todos de uma vez, ou muta todos). */
bool grupo_get_ativo(int grupo_id);
void grupo_alternar(int grupo_id);

/* Aplica a mascara de mute groups recebida da mesa aos canais da interface. */
void estado_aplicar_mgmask(uint32_t mgmask);

/* Chamada sempre que o usuario muda o estado de um canal pela tela.
 * Hoje e apenas um stub (nao existe conexao real com a mesa ainda).
 * Quando o protocolo da Ui24R for implementado, envie o comando de
 * mute real para o canal "canal" (numero de 1-22, ver data.h) aqui. */
void enviar_mute_para_mesa(int numero_canal, bool ativo);
