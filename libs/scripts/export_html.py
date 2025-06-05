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

import argparse
import os
import sys

import csv
import configparser
import numpy as np

from imageio import imread
from nnabla.logger import logger


def digit_str(value):
    return str('{:,}'.format(value))


def dataset_image_report(
        dataset_path,
        dataset_name,
        rows,
        types,
        variables,
        starts,
        dims,
        labels,
        report):
    # title
    title = 'Examples of variable '
    index = []
    for i, (type, variable) in enumerate(zip(types, variables)):
        if type == 'Image' or type == 'Array of image':
            title += (', ' if len(index) else '') + variable
            index.append(i)

    if len(index) <= 0:
        return

    report.html.append('<p>' + title + ' in "' + dataset_name + '"</p>')
    row_index = np.random.permutation(len(rows) - 1)[:10]

    imgs = ''
    for ir in row_index:
        for idx in index:
            for id in range(dims[idx]):
                img_path = os.path.join(
                    dataset_path, rows[ir + 1][starts[idx] + id])
                imgs += '<img src="' + img_path + '">'
            imgs += ' '
        imgs += ' '

    report.html.append(imgs)


def dataset_report(args, config, report):
    num_dataset = int(config['Dataset']['Num'])

    for i in range(num_dataset):
        dataset_config = config['Dataset_' + str(i)]

        # try:
        dataset_uri = dataset_config['URI']
        with open(dataset_uri, 'r', encoding='utf-8-sig') as f:
            # title
            dataset_name = dataset_config['Name']
            report.html.append('<h2>Dataset : ' + dataset_name + '</h2>')
            report.html.append('<ul>')

            reader = csv.reader(f)
            rows = [row for row in reader]

            report.html.append(
                '<li>Number of data : ' +
                digit_str(
                    len(rows) -
                    1) +
                '</li>')

            starts = []
            variables = []
            types = []
            labels = []
            dims = []
            for ic, (cell, data) in enumerate(zip(rows[0], rows[1])):
                label = cell.split(':')
                variable = label[0].split('__')[0]
                label = label[1] if len(label) > 1 else ''
                type = os.path.splitext(data)[1].lower()
                if (type == '.csv'):
                    type = 'Matrix'
                elif type in ['.bmp', '.jpeg', '.jpg', '.png', '.gif', '.tif']:
                    type = 'Image'
                else:
                    type = 'Scalar'
                if len(variables) and variables[-1] == variable:
                    dims[-1] += 1
                    labels[-1].append(label)
                    if types[-1] == 'Matrix':
                        types[-1] = 'Tensor'
                    elif types[-1] == 'Image':
                        types[-1] = 'Array of image'
                    else:
                        types[-1] = 'Vector'
                else:
                    starts.append(ic)
                    variables.append(variable)
                    types.append(type)
                    dims.append(1)
                    labels.append([label])

            dataset_path = os.path.split(dataset_uri)[0]
            for start, variable, type, dim, label in zip(
                    starts, variables, types, dims, labels):
                report.html.append('<li>Variable : ' +
                                   variable +
                                   (' (' +
                                    label[0] +
                                       ')' if len(label) == 1 and label[0] else '') +
                                   '</li>')
                report.html.append('<ul>')
                report.html.append('<li>Type : ' + type + '</li>')

                if type != 'Scalar':
                    if type == 'Vector':
                        shape = str(dim)
                    elif type == 'Image' or type == 'Array of image':
                        im = imread(os.path.join(dataset_path, rows[1][start]))
                        shape = str(im.shape[0]) + ', ' + str(im.shape[1])
                        if len(im.shape) > 2:
                            shape = str(im.shape[2]) + ', ' + shape
                        else:
                            shape = '1, ' + shape
                        if dim > 1:
                            shape = str(dim) + ', ' + shape
                    else:
                        with open(os.path.join(dataset_path, rows[1][start]), 'r', encoding='utf-8-sig') as f:
                            data_reader = csv.reader(f)
                            data_rows = [row for row in data_reader]
                            shape = str(len(data_rows)) + ', ' + \
                                str(len(data_rows[0]))

                    report.html.append('<li>Shape : ' + shape + '</li>')
                report.html.append('</ul>')
            report.html.append('</ul>')

            dataset_image_report(
                dataset_path,
                dataset_name,
                rows,
                types,
                variables,
                starts,
                dims,
                labels,
                report)
        # except:
        #     pass


def network_report(args, config, report):
    if 'NumNetwork' in config['Global']:
        num_network = int(config['Global']['NumNetwork'])
        network_names = [config['Global']
                         ['Network_' + str(i)] for i in range(num_network)]
        prefixes = [n + '_' for n in network_names]
    else:
        num_network = 1
        network_names = ['Main']
        prefixes = ['']

    for name, prefix in zip(network_names, prefixes):
        global_config = config[prefix + 'Network_Global']

        # title
        report.html.append('<h2>Network Architecture : ' + name + '</h2>')

        # network image
        img_path = os.path.join(
            os.path.dirname(
                args.input),
            global_config['Image'])
        report.html.append('<img src="' + img_path + '">')

        # statistics
        statistics_config = config[prefix + 'Statistics']
        num_statistics = int(
            statistics_config['NumStatistics'] if 'NumStatistics' in statistics_config else 0)
        if num_statistics:
            report.html.append(
                '<table border=1><tr><th>Type</th><th>Value</th></tr>')
            for i2 in range(num_statistics):
                key = 'Statistics_' + str(i2)
                report.html.append('<tr><td>' +
                                   statistics_config[key +
                                                     '_Name'] +
                                   '</td><td>' +
                                   digit_str(int(statistics_config[key +
                                                                   '_Sum'])) +
                                   '</td></tr>')
            report.html.append('</table>')


def config_report(args, config, report):
    epoch = int(config['Config']['MaxEpoch'])
    batch_size = int(config['Config']['BatchSize'])
    num_optimizer = int(config['Config']['NumOptimizer'])

    # optimizer
    for i in range(num_optimizer):
        optimizer_config = config['Optimizer_' + str(i)]

        report.html.append(
            '<h2>Training Procedure : ' +
            optimizer_config['Optimizer_Name'] +
            '</h2>')
        report.html.append(
            '<p>Optimize network "' +
            optimizer_config['Optimizer_NetworkName'] +
            '" using "' +
            optimizer_config['Optimizer_DatasetName'] +
            '" dataset.</p>')

        report.html.append('<ul>')
        # batch size
        accum_times = int(optimizer_config['UpdateInterval'])
        report.html.append('<li>Batch size : ' +
                           str(batch_size * accum_times) + '</li>')
        if accum_times > 1:
            report.html.append('<ul>')
            report.html.append(
                '<li>by accumulating the result of batch-size ' +
                str(batch_size) +
                ' by ' +
                str(accum_times) +
                ' times.</li>')
            report.html.append('</ul>')

        # solver
        report.html.append(
            '<li>Solver : ' +
            optimizer_config['SolverName'] +
            '</li>')
        report.html.append('<ul>')
        lr_multiplier = float(optimizer_config['LearningRateMultiplier'])
        lr_update_interval = int(
            optimizer_config['LearningRateUpdateInterval'])

        # solver : range
        range_from = int(
            optimizer_config['RangeFrom']) if 'RangeFrom' in optimizer_config else -1
        range_to = int(optimizer_config['RangeTo']
                       ) if 'RangeTo' in optimizer_config else -1
        range_unit = int(
            optimizer_config['RangeUnit']) if 'RangeUnit' in optimizer_config else 0
        if range_from >= 0 or range_to >= 0:
            range_unit_text = 'iteration' if range_unit else 'epoch'
            if range_from >= 0:
                range_text = 'Processed from {} {}'.format(
                    range_from, range_unit_text)
                if range_to >= 0:
                    range_text += ' to {} {}'.format(range_to, range_unit_text)
            else:
                range_text = 'Processed until {} {}'.format(
                    range_to, range_unit_text)
            report.html.append('<li>' + range_text + '.</li>')

        # solver : learning rate
        lr_multiplier = float(optimizer_config['LearningRateMultiplier'])
        lr_update_scheduler = optimizer_config['LearningRateScheduler'] if 'LearningRateScheduler' in optimizer_config else 'Exponential'
        lr_update_interval = int(
            optimizer_config['LearningRateUpdateInterval'])
        lr_update_interval_unit = int(
            optimizer_config['LearningRateUpdateIntervalUnit'])
        lr_power = float(optimizer_config['LearningRatePower']
                         ) if 'LearningRatePower' in optimizer_config else 1.0
        lr_steps = optimizer_config['LearningRateSteps'] if 'LearningRateSteps' in optimizer_config else ''
        lr_warmup_scheduler = optimizer_config['LearningRateWarmupScheduler'] if 'LearningRateWarmupScheduler' in optimizer_config else 'None'
        lr_warmup_length = int(
            optimizer_config['LearningRateWarmupLength']) if 'LearningRateWarmupLength' in optimizer_config else 0
        lr_warmup_unit = int(
            optimizer_config['LearningRateWarmupUnit']) if 'LearningRateWarmupUnit' in optimizer_config else 0

        learning_rate_text = 'Learning rate' + \
            ('(' + optimizer_config['SolverParameterName_0'] + ') ' if optimizer_config['SolverParameterName_0'] != 'LearningRate' else '')
        learning_rate_text += ': ' + optimizer_config['SolverParameter_0']
        report.html.append('<li>' + learning_rate_text + '</li>')

        report.html.append('<ul>')
        if lr_update_scheduler == 'Exponential':
            if lr_multiplier != 1.0:
                report.html.append('<li>decayed every ' +
                                   digit_str(lr_update_interval) +
                                   ' ' +
                                   ('iteration' if lr_update_interval_unit else 'epoch') +
                                   ' using an exponential rate of ' +
                                   str(lr_multiplier) +
                                   ' .</li>')
        elif lr_update_scheduler == 'Cosine':
            report.html.append('<li>decayed with cosine annealing.</li>')
        elif lr_update_scheduler == 'Polynomial':
            report.html.append(
                '<li>decayed with polynomial learning rate decay (power = {}).</li>'.format(lr_power))
        elif lr_update_scheduler == 'Step':
            if lr_steps != '':
                report.html.append('<li>decayed at ' +
                                   lr_steps +
                                   ' ' +
                                   ('iteration' if lr_update_interval_unit else 'epoch') +
                                   ' using an exponential rate of ' +
                                   str(lr_multiplier) +
                                   ' .</li>')
        if lr_warmup_scheduler != 'None' and lr_warmup_length > 0:
            report.html.append(
                '<li>warmed up linearly over the first {} {}.</li>'.format(
                    lr_warmup_length,
                    'iterations' if range_unit else 'epochs'))
        report.html.append('</ul>')

        # solver : other parameters
        num_solver_parameter = int(optimizer_config['SolverParameterNum'])
        for ip in range(num_solver_parameter - 1):
            report.html.append('<li>' +
                               optimizer_config['SolverParameterName_' +
                                                str(ip +
                                                    1)] +
                               ' : ' +
                               optimizer_config['SolverParameter_' +
                                                str(ip +
                                                    1)] +
                               '</li>')
        report.html.append('</ul>')

        # weight decay
        weight_decay = float(optimizer_config['WeightDecay'])
        report.html.append('<li>Weight decay ' + (': ' + str(weight_decay)
                                                  if weight_decay != 0 else 'is not applied.</li>'))
        report.html.append('</ul>')


def experimental_result_report(args, config, report):
    if 'Result' not in config:
        return

    if 'LearningCurve_Image' in config['Result']:
        # title
        report.html.append('<h2>Experimental Result : Learning Curve</h2>')

        # learning curve image
        img_path = os.path.join(
            os.path.dirname(
                args.input),
            config['Result']['LearningCurve_Image'])
        report.html.append('<img src="' + img_path + '">')

    num_confusion_matrix = int(config['Result']['NumConfusionMatrix'])
    if num_confusion_matrix:
        # title
        report.html.append('<h2>Experimental Result : Evaluation</h2>')

        # executor
        num_executor = int(config['Config']['NumExecutor'])
        for i in range(num_executor):
            executor_config = config['Executor_' + str(i)]
            report.html.append(
                '<p>Evaluate network "' +
                executor_config['Executor_NetworkName'] +
                '" using "' +
                executor_config['Executor_DatasetName'] +
                '" dataset.</p>')
            report.html.append('<ul>')

            num_evaluation = int(executor_config['NumEvaluations'])
            if num_evaluation > 1:
                repeat_evaluation_type = int(
                    executor_config['RepeatEvaluationType'])
                with_back_propagation = int(
                    executor_config['NeedBackPropagation'])
                report.html.append('<li>The calculation is executed ' +
                                   str(num_evaluation) +
                                   ' times for each data' +
                                   (' with back propagation' if with_back_propagation else '.') +
                                   '</li>')

                report.html.append('<li>The ' +
                                   ('last' if repeat_evaluation_type else 'average') +
                                   ' value is used as a final output.</li>')
            report.html.append('</ul>')

        for i in range(num_confusion_matrix):
            confusion_matrix_file_name = config['Result']['ConfusionMatrix_' + str(
                i)]
            confusion_matrix_path = os.path.join(
                os.path.dirname(args.input), confusion_matrix_file_name)

            variable_name = os.path.split(confusion_matrix_path)[1].replace(
                'confusion_matrix_', '').replace('.csv', '')
            report.html.append('<p>Variable : ' + variable_name + '</p>')
            report.html.append('<ul>')
            with open(confusion_matrix_path, 'r', encoding='utf-8-sig') as f:
                reader = csv.reader(f)

                rows = [row for row in reader]
                for i in range(4):
                    row = rows[len(rows) - 4 + i]
                    report.html.append(
                        '<li>' + row[0] + ' : ' + row[1] + '</li>')
            report.html.append('</ul>')


def references(args, config, report):
    report.html.append('<h2>References</h2>')
    report.html.append('<ul>')

    report.html.append(
        '<li>Sony Corporation. Neural Network Console : Not just train and evaluate. You can design neural networks with fast and intuitive GUI. https://dl.sony.com/</li>')

    report.html.append('<li>Sony Corporation. Neural Network Libraries : An open source software to make research, development and implementation of neural network more efficient. https://nnabla.org/</li>')

    num_reference = int(config['Reference']['NumReference'])
    for i in range(num_reference):
        report.html.append('<li>' +
                           config['Reference']['ReferenceLayer_' +
                                               str(i)] +
                           ' - ' +
                           config['Reference']['Reference_' +
                                               str(i)].replace('\\n', ',\t') +
                           '</li>')
    report.html.append('</ul>')


def create_html_report(args):
    # open config
    logger.log(99, 'Loading config file...')
    config = configparser.ConfigParser()
    config.read(args.input)

    # title slide
    class Report:
        pass
    report = Report()
    report.html = []

    logger.log(99, 'Creating title ...')
    report.html.append('<h1>' + config['Global']['ProjectName'] + '</h1>')
    report.html.append(
        '<p>' +
        config['Description']['Text'].replace(
            '\\n',
            '</p><p>') +
        '</p>')

    # dataset report
    logger.log(99, 'Exporting dataset report...')
    dataset_report(args, config, report)

    # network report
    logger.log(99, 'Exporting network report...')
    network_report(args, config, report)

    # config report
    logger.log(99, 'Exporting configuration report...')
    config_report(args, config, report)

    # experimental result report
    logger.log(99, 'Exporting experimental result...')
    experimental_result_report(args, config, report)

    # references
    logger.log(99, 'Exporting references...')
    references(args, config, report)

    # save presentation
    logger.log(99, 'Saving html file...')
    report.html = [line + '\n' for line in report.html]
    with open(args.output, mode='wt', encoding='utf-8') as f:
        f.writelines(report.html)
    logger.log(99, 'Export html completed successfully.')


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        '-i', '--input', help='path to report.ini', required=True)
    parser.add_argument(
        '-o', '--output', help='output html file name', required=True)
    parser.set_defaults(func=create_html_report)

    args = parser.parse_args()
    args.func(args)


if __name__ == '__main__':
    main()
