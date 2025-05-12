document.addEventListener('DOMContentLoaded', function() {
    const configuracao = {
        idCanal: '2572428',
        chaveApi: 'H2T7PYFVDDTGOXK1',
        chaveEscritaApi: 'R6EV3MSGTMAN99QA',
        numeroResultados: 12,
        intervaloAtualizacao: 30000 // 30 segundos
    };

    const estado = {
        recorde: { nome: '', pontuacao: 0 },
        carregando: false
    };

    const elementos = {
        corpoTabela: document.querySelector('#dataTable tbody'),
        elementoRecorde: document.getElementById('recorde')
    };

  
    let dadosAnteriores = [];

    const formatarDados = (dados) => {
        return dados
            .map(dado => ({
                nome: dado.field2 || 'Anônimo',
                pontuacao: parseInt(dado.field1, 10) || 0,
                data: new Date(dado.created_at)
            }))
            .filter(item => item.pontuacao > 0) 
            .sort((a, b) => b.pontuacao - a.pontuacao || b.data - a.data);
    };

    const renderizarTabela = (dados) => {
        elementos.corpoTabela.innerHTML = dados.map((item, indice) => `
            <tr ${indice === 0 ? 'class="destaque"' : ''}>
                <td>${item.nome}</td>
                <td>${item.pontuacao}</td>
            </tr>
        `).join('');
    };

    const atualizarRecorde = (dados) => {
        if (dados.length > 0 && dados[0].pontuacao > estado.recorde.pontuacao) {
            estado.recorde = { nome: dados[0].nome, pontuacao: dados[0].pontuacao };
            elementos.elementoRecorde.textContent = `${estado.recorde.nome}: ${estado.recorde.pontuacao} pontos`;

            fetch(`https://api.thingspeak.com/update?api_key=${configuracao.chaveEscritaApi}&field3=${estado.recorde.pontuacao}`)
                .catch(erro => console.error('Erro ao atualizar recorde:', erro));
        }
    };

    const buscarDados = async () => {
        if (estado.carregando) return;
        estado.carregando = true;

        try {
            const resposta = await fetch(
                `https://api.thingspeak.com/channels/${configuracao.idCanal}/feeds.json?api_key=${configuracao.chaveApi}&results=${configuracao.numeroResultados}`
            );
        
            if (!resposta.ok) throw new Error('Erro na resposta da API');
        
            const dados = await resposta.json();
            const dadosFormatados = formatarDados(dados.feeds);
        
            if (dadosFormatados.length > 0) {
                dadosAnteriores = dadosFormatados;
                renderizarTabela(dadosFormatados);
                atualizarRecorde(dadosFormatados);
            } else {
                console.warn('Nenhuma pontuação válida encontrada. Mantendo os resultados anteriores.');
                if (dadosAnteriores.length > 0) {
                    renderizarTabela(dadosAnteriores);
                }
            }
        } catch (erro) {
            console.error('Erro ao buscar dados:', erro);
          
            if (dadosAnteriores.length > 0) {
                renderizarTabela(dadosAnteriores);
            }
        } finally {
            estado.carregando = false;
        }
    };

    buscarDados();

    const idIntervalo = setInterval(buscarDados, configuracao.intervaloAtualizacao);

    window.addEventListener('beforeunload', () => {
        clearInterval(idIntervalo);
    });
});