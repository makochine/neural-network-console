# Copyright 2018,2019,2020,2021 Sony Corporation.
# Copyright 2021,2022,2023,2024,2025 Sony Group Corporation.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.


def plot_series_command(args):
    import nnabla.monitor as M
    plot_any_command(args, M.plot_series)
    return True


def plot_timer_command(args):
    import nnabla.monitor as M
    format_unit = dict(
        s='seconds',
        m='minutes',
        h='hours',
        d='days',
        )

    if not args.ylabel:
        if args.elapsed:
            args.ylabel = 'Total elapsed time [{}]'.format(
                format_unit[args.time_unit])
        else:
            args.ylabel = 'Elapsed time [{}/iter]'.format(
                format_unit[args.time_unit])
    plot_any_command(args, M.plot_time_elapsed, dict(
        elapsed=args.elapsed, unit=args.time_unit))
    return True


def plot_any_command(args, plot_func, plot_func_kwargs=None):
    if plot_func_kwargs is None:
        plot_func_kwargs = {}
    if args.outfile:
        import matplotlib
        matplotlib.use('Agg')
    if args.label is None:
        args.label = []

    import matplotlib.pyplot as plt
    plt.figure()
    for i, inp in enumerate(args.inputs):
        label = args.label[i] if len(args.label) > i else inp
        plot_func(inp, plot_kwargs=dict(label=label), **plot_func_kwargs)
    plt.legend(loc='best')
    if args.title:
        plt.title(args.title)
    if args.xlabel:
        plt.xlabel(args.xlabel)
    if args.ylabel:
        plt.ylabel(args.ylabel)
    if args.xlim_min is not None:
        plt.xlim(left=args.xlim_min)
    if args.xlim_max is not None:
        plt.xlim(right=args.xlim_max)
    if args.ylim_min is not None:
        plt.ylim(bottom=args.ylim_min)
    if args.ylim_max is not None:
        plt.ylim(top=args.ylim_max)
    if args.outfile:
        plt.savefig(args.outfile)
        return
    plt.show()
