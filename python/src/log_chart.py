
from typing import List
import csv
import matplotlib.pyplot as plt
import numpy as np


class PlayerGameResults:
    def __init__(self, res: str):
        prop = res.split('/')

        self.wins = int(prop[0])
        self.winsStartedGame = int(prop[1])

class GameResults:
    def __init__(self, row: List[str]):
        self.iteration = int(row[0])
        self.draw = int(row[1])
        self.player1 = PlayerGameResults(row[2])
        self.player2 = PlayerGameResults(row[3])


def build_improvement_chart():
    grs = []
    # Win rate
    win_rate = []

    with open('log/azr-improvement-log.txt') as csv_file:
        csv_reader = csv.reader(csv_file, delimiter=',')
        for row in csv_reader:
            grs.append(GameResults(row))
        for gr in grs:
            win_rate.append(gr.player1.wins / (gr.player1.wins + gr.player2.wins) * 100)

    plt.title('Delež zmag novega modela proti staremu')
    plt.plot(win_rate, linestyle='--', marker='o')
    plt.axhline(y=55, color='r', label='Prag izbolšanega modela')
    plt.ylabel('Delež zmag %')
    plt.xlabel('Iteracija')
    plt.savefig('charts/win_rate_new_vs_old')
    plt.show()


def build_benchmark_chart():
    iteration_grs = []
    random_grs = []
    script_grs = []

    with open('log/azr-benchmark-log.txt') as csv_file:
        csv_reader = csv.reader(csv_file, delimiter=',')
        for row in csv_reader:
            iteration_grs.append(int(row[0]))
            random_grs.append(GameResults(row[0:4]))
            script_grs.append(GameResults(row[0:1] + row[4:]))

    # Delež zmag
    random_win_rate = []
    for gr in random_grs:
        random_win_rate.append(gr.player1.wins / (gr.player1.wins + gr.player2.wins) * 100)

    plt.title('Delež zmag novega modela proti naključnemu igralcu')
    plt.plot(iteration_grs, random_win_rate, linestyle='--', marker='o')
    plt.axhline(y=50, color='r', label='Prag večjega deleža')
    plt.ylabel('Delež zmag %')
    plt.xlabel('Iteracija')
    plt.savefig('charts/win_rate_new_vs_random')
    plt.show()

    # Delež zmag
    script_win_rate = []
    for gr in script_grs:
        script_win_rate.append(gr.player1.wins / (gr.player1.wins + gr.player2.wins) * 100)

    plt.title('Delež zmag novega modela proti programiranemu igralcu')
    plt.plot(iteration_grs, script_win_rate, linestyle='--', marker='o')
    plt.axhline(y=50, color='r', label='Prag večjega deleža')
    plt.ylabel('Delež zmag %')
    plt.xlabel('Iteracija')
    plt.savefig('charts/win_rate_new_vs_script')
    plt.show()


def build_NN_batch_speed_chart():
    batch_size = []
    avg_time = []

    with open('log/batch-size-perf.txt') as csv_file:
        csv_reader = csv.reader(csv_file, delimiter=',')
        for row in csv_reader:
            batch_size.append(int(row[0]))
            avg_time.append(int(row[1]))

    plt.title('Čas obdelave primera')
    plt.plot(batch_size, avg_time, linestyle='--', marker='o')
    for a, b in zip(batch_size, avg_time):
        plt.annotate(str(a), xy=(a+30, b))
    plt.ylabel('Povprečni čas obdelave primera (ms)')
    plt.xlabel('Velikost serije')
    plt.savefig('charts/batch-size-perf')
    plt.show()


def autolabel(rects, ax):
    """Attach a text label above each bar in *rects*, displaying its height."""
    for rect in rects:
        height = rect.get_height()
        ax.annotate('{:2.1f}'.format(height),
                    xy=(rect.get_x() + rect.get_width() / 2, height),
                    xytext=(0, 3),  # 3 points vertical offset
                    textcoords="offset points",
                    ha='center', va='bottom')


def build_model_perf_chart():
    labels_mcts = ['32', '64', '128', '256', '512']
    labels_count = len(labels_mcts)

    models_label = ['B 20, P', 'B 20, P+N', 'B 5, P', 'B 5, P+N']
    models_result = [[] for _ in range(4)]

    with open('log/results-short.txt') as csv_file:
        csv_reader = csv.reader(csv_file, delimiter='|')

        next(csv_reader, None)  # Skip header
        for row in csv_reader:
            for m in range(4):
                prop = row[m].split(',')
                model_data = prop[1].split('/')
                script_data = prop[2].split('/')
                result = int(model_data[0]) / (int(model_data[0]) + int(script_data[0]))
                models_result[m].append(result * 100)

    x = np.arange(labels_count)  # the label locations
    width = 0.20  # the width of the bars

    fig, ax = plt.subplots()
    fig.set_figheight(6)
    fig.set_figwidth(10)

    for i, mr in enumerate(models_result):
        rects = ax.bar(x + i * width - width * 1.5, mr, width, label=models_label[i])
        autolabel(rects, ax)

    ax.set_xticks(x)
    ax.set_xticklabels(labels_mcts)
    ax.legend()

    plt.title('Delež zmag modelov')
    plt.ylabel('Delež zmag (%)')
    plt.xlabel('Število MCTS preiskav')
    plt.savefig('charts/models_vs_script_perf')
    plt.show()


def build_model_nn_training_error_chart():
    models_label = ['B 20, P', 'B 20, P+N', 'B 5, P', 'B 5, P+N']
    models_result_policy = [[] for _ in range(4)]
    models_result_value = [[] for _ in range(4)]

    models_files = ['log/nn-20-ss.txt', 'log/nn-20-sr.txt', 'log/nn-5-ss.txt', 'log/nn-5-sr.txt']

    for i, mf in enumerate(models_files):
        m_pi = models_result_policy[i]
        m_v = models_result_value[i]
        with open(mf) as csv_file:
            csv_reader = csv.reader(csv_file, delimiter=',')
            for row in csv_reader:
                m_pi.append(float(row[0]))
                m_v.append(float(row[1]))

    iteration_label = [i + 1 for i in range(len(models_result_policy[0]))]

    #  Graf vrednosti potez
    plt.title('Napredek učenja modelov')
    plt.ylabel('Napaka na vektorju potez')
    plt.xlabel('Iteracija')

    for i in range(4):
        plt.plot(iteration_label, models_result_policy[i], label=models_label[i])

    plt.legend()
    plt.savefig('charts/models_policy_nn_training')
    plt.show()


    #  Graf vrednosti stanja
    plt.title('Napredek učenja modelov')
    plt.ylabel('Napaka na vrednosti stanja')
    plt.xlabel('Iteracija')

    for i in range(4):
        plt.plot(iteration_label, models_result_value[i], label=models_label[i])

    plt.legend()
    plt.savefig('charts/models_value_nn_training')
    plt.show()


if __name__ == "__main__":
    # build_model_nn_training_error_chart()
    # build_model_perf_chart()
    # build_NN_batch_speed_chart()
    build_improvement_chart()
    build_benchmark_chart()
