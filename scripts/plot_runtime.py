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

    datas = {}
    runtime = 50
    fib_modes = ["iteration", "fast_doubling",
                 "clz_fast_doubling", "my_bn_iteration", "ref_bn_iteration", "ref_bn_doubling"]

    # run program for runtime
    modes = [3, 4, 5]
    # run test for every mode
    for mode in modes:
        temp = []
        # run test on each mode for runtime
        for _ in range(runtime):
            subprocess.run(
                f"sudo taskset -c 15 ./client_test {mode} > runtime.txt", shell=True)
            _data = np.loadtxt("runtime.txt", dtype=float)
            temp.append(_data)
        temp = data_processing(temp)
        datas[mode] = temp

    # plot
    _, ax = plt.subplots()
    plt.grid()
    for k, v in datas.items():
        ax.plot(np.arange(datas[k].shape[-1]),
                datas[k], marker='+',  markersize=3, label=fib_modes[k])
    plt.legend(loc='upper left')
    plt.ylabel("kernel runtime (ns)")
    plt.xlabel("F(n)")
    plt.savefig("runtime.png")


if __name__ == "__main__":
    main()
