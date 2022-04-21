import numpy as np
import subprocess
import matplotlib.pyplot as plt

np.seterr(divide='ignore', invalid='ignore')


def outlier_filter(datas, threshold=2):
    datas = np.array(datas)

    z = np.abs((datas - datas.mean() + 1e-7) / (datas.std() + 1e-7))
    return datas[z < threshold]


def data_processing(data):

    data = np.array(data)
    _, test_samples = data.shape
    result = np.zeros(test_samples)

    for i in range(test_samples):
        result[i] = outlier_filter(data[:, i]).mean()

    return result


def main():

    datas = []
    runtime = 50
    fib_modes = ["iteration", "fast_doubling",
                 "clz_fast_doubling", "bn_normal", "bn_fast_doubling"]

    # run program for runtime
    modes = 3
    # run test for every mode
    for mode in range(modes):
        temp = []
        # run test on each mode for runtime
        for _ in range(runtime):
            subprocess.run(
                f"sudo taskset -c 15 ./client_test {mode} > runtime.txt", shell=True)
            _data = np.loadtxt("runtime.txt", dtype=float)
            temp.append(_data)
        temp = data_processing(temp)
        datas.append(temp)

    # plot
    _, ax = plt.subplots()
    plt.grid()
    for i, data in enumerate(datas):
        ax.plot(np.arange(data.shape[-1]),
                data, marker='+',  markersize=3, label=fib_modes[i])
    plt.legend(loc='upper left')
    plt.ylabel("kernel runtime (ns)")
    plt.xlabel("F(n)")
    plt.savefig("runtime.png")


if __name__ == "__main__":
    main()
