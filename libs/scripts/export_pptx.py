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

from pptx.api import Presentation
from pptx.util import Cm, Pt

from imageio import imread
from nnabla.logger import logger


def digit_str(value):
    return str('{:,}'.format(value))


def set_table_font_size(table, size):
    for row in table.rows:
        for cell in row.cells:
            for paragraph in cell.text_frame.paragraphs:
                for run in paragraph.runs:
                    run.font.size = size


def dataset_image_report(
        dataset_path,
        dataset_name,
        rows,
        types,
        variables,
        starts,
        dims,
        labels,
        prs):
    # slide title
    title = 'Dataset : Examples of variable '
    index = []
    for i, (type, variable) in enumerate(zip(types, variables)):
        if type == 'Image' or type == 'Array of image':
            title += (', ' if len(index) else '') + variable
            index.append(i)

    if len(index) <= 0:
        return

    slide_layout = prs.slide_layouts[5]  # layout only with title
    slide = prs.slides.add_slide(slide_layout)

    slide.shapes.title.text = title + ' in "' + dataset_name + '"'

    row_index = np.random.permutation(len(rows) - 1)
    top = 2.7
    left = 1.27
    height = 0

    for ir in row_index:
        width = 0
        image_files = []
        rect = []
        for idx in index:
            for id in range(dims[idx]):
                img_path = os.path.join(
                    dataset_path, rows[ir + 1][starts[idx] + id])
                ppc = 480 / 10.06
                im = imread(img_path)
                xscale = 2 * ppc / im.shape[1]
                yscale = 2 * ppc / im.shape[0]
                scale = xscale if xscale < yscale else yscale
                if scale > 1.0:
                    scale = 1.0
                image_files.append(img_path)
                w = scale * im.shape[1] / ppc
                h = scale * im.shape[0] / ppc
                rect.append([w, h])
                width += w
                if height < h:
                    height = h

        for img_path, r in zip(image_files, rect):
            slide.shapes.add_picture(
                img_path, Cm(left), Cm(top), width=Cm(
                    r[0]), height=Cm(
                    r[1]))
            left += r[0]

        left += 0.25

        if left + width > 1.27 + 22.86:
            left = 1.27
            top += height + 0.25
            if top + height > 2.7 + 10.06:
                break


def dataset_report(args, config, prs):
    num_dataset = int(config['Dataset']['Num'])

    for i in range(num_dataset):
        dataset_config = config['Dataset_' + str(i)]

        try:
            dataset_uri = dataset_config['URI']
            with open(dataset_uri, 'r', encoding='utf-8-sig') as f:
                # layout with single placeholder
                slide_layout = prs.slide_layouts[1]
                slide = prs.slides.add_slide(slide_layout)

                # slide title
                dataset_name = dataset_config['Name']
                slide.shapes.title.text = 'Dataset : ' + dataset_name

                reader = csv.reader(f)
                rows = [row for row in reader]

                text_frame = slide.shapes.placeholders[1].text_frame
                text_frame.text = 'Number of data : ' + \
                    digit_str(len(rows) - 1)

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
                    p = text_frame.add_paragraph()
                    p.text = 'Variable : ' + variable + \
                        (' (' + label[0] + ')' if len(label) == 1 and label[0] else '')
                    p.level = 1

                    p = text_frame.add_paragraph()
                    p.text = 'Type : ' + type
                    p.level = 2

                    if type != 'Scalar':
                        p = text_frame.add_paragraph()
                        if type == 'Vector':
                            shape = str(dim)
                        elif type == 'Image' or type == 'Array of image':
                            im = imread(
                                os.path.join(
                                    dataset_path,
                                    rows[1][start]))
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
                                shape = str(len(data_rows)) + \
                                    ', ' + str(len(data_rows[0]))

                        p.text = 'Shape : ' + shape
                        p.level = 2

                dataset_image_report(
                    dataset_path,
                    dataset_name,
                    rows,
                    types,
                    variables,
                    starts,
                    dims,
                    labels,
                    prs)
        except BaseException:
            pass


def network_report(args, config, prs):
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

        slide_layout = prs.slide_layouts[5]  # layout only with title
        slide = prs.slides.add_slide(slide_layout)

        # slide title
        slide.shapes.title.text = 'Network Architecture : ' + name

        # statistics
        statistics_config = config[prefix + 'Statistics']
        num_statistics = int(
            statistics_config['NumStatistics'] if 'NumStatistics' in statistics_config else 0)
        if num_statistics:
            table = slide.shapes.add_table(
                num_statistics + 1, 2, Cm(1.27 + 22.86 - 8), Cm(2.7), Cm(8), Cm(8)).table
            table.cell(0, 0).text = 'Type'
            table.cell(0, 1).text = 'Value'
            for i2 in range(num_statistics):
                key = 'Statistics_' + str(i2)
                table.cell(i2 + 1, 0).text = statistics_config[key + '_Name']
                table.cell(
                    i2 + 1, 1).text = digit_str(int(statistics_config[key + '_Sum']))

            set_table_font_size(table, Pt(12))

        # network image
        img_path = os.path.join(
            os.path.dirname(
                args.input),
            global_config['Image'])
        ppc = 480 / 10.06
        im = imread(img_path)
        xscale = (22.86 if not num_network else 22.86 - 8) * ppc / im.shape[1]
        yscale = 10.06 * ppc / im.shape[0]
        scale = xscale if xscale < yscale else yscale
        if scale > 1.0:
            scale = 1.0
        slide.shapes.add_picture(
            img_path,
            Cm(1.27),
            Cm(2.7),
            width=Cm(
                scale *
                im.shape[1] /
                ppc),
            height=Cm(
                scale *
                im.shape[0] /
                ppc))


def config_report(args, config, prs):
    epoch = int(config['Config']['MaxEpoch'])
    batch_size = int(config['Config']['BatchSize'])
    num_optimizer = int(config['Config']['NumOptimizer'])

    # optimizer
    for i in range(num_optimizer):
        optimizer_config = config['Optimizer_' + str(i)]

        slide_layout = prs.slide_layouts[1]  # layout with single placeholder
        slide = prs.slides.add_slide(slide_layout)

        slide.shapes.title.text = 'Training Procedure : ' + \
            optimizer_config['Optimizer_Name']
        text_frame = slide.shapes.placeholders[1].text_frame
        text_frame.text = 'Optimize network "' + \
            optimizer_config['Optimizer_NetworkName'] + '" using "' + optimizer_config['Optimizer_DatasetName'] + '" dataset.'

        # batch size
        accum_times = int(optimizer_config['UpdateInterval'])
        p = text_frame.add_paragraph()
        p.text = 'Batch size : ' + str(batch_size * accum_times)
        p.level = 1
        if accum_times > 1:
            p = text_frame.add_paragraph()
            p.text = 'by accumulating the result of batch-size ' + \
                str(batch_size) + ' by ' + str(accum_times) + ' times.'
            p.level = 2

        # solver
        p = text_frame.add_paragraph()
        p.text = 'Solver : ' + optimizer_config['SolverName']
        p.level = 1

        # solver : range
        range_from = int(
            optimizer_config['RangeFrom']) if 'RangeFrom' in optimizer_config else -1
        range_to = int(optimizer_config['RangeTo']
                       ) if 'RangeTo' in optimizer_config else -1
        range_unit = int(
            optimizer_config['RangeUnit']) if 'RangeUnit' in optimizer_config else 0
        if range_from >= 0 or range_to >= 0:
            p = text_frame.add_paragraph()
            range_unit_text = 'iteration' if range_unit else 'epoch'
            if range_from >= 0:
                range_text = 'Processed from {} {}'.format(
                    range_from, range_unit_text)
                if range_to >= 0:
                    range_text += ' to {} {}'.format(range_to, range_unit_text)
            else:
                range_text = 'Processed until {} {}'.format(
                    range_to, range_unit_text)
            p.text = range_text + '.'
            p.level = 2

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
        p = text_frame.add_paragraph()
        learning_rate_text = 'Learning rate' + \
            ('(' + optimizer_config['SolverParameterName_0'] + ') ' if optimizer_config['SolverParameterName_0'] != 'LearningRate' else '')
        learning_rate_text += ': ' + optimizer_config['SolverParameter_0']
        p.text = learning_rate_text
        p.level = 2

        if lr_update_scheduler == 'Exponential':
            if lr_multiplier != 1.0:
                p = text_frame.add_paragraph()
                p.text = 'decayed every ' + digit_str(lr_update_interval) + ' ' + (
                    'iteration' if lr_update_interval_unit else 'epoch') + ' using an exponential rate of ' + str(lr_multiplier) + ' .'
                p.level = 3
        elif lr_update_scheduler == 'Cosine':
            p = text_frame.add_paragraph()
            p.text = 'decayed with cosine annealing.'
            p.level = 3
        elif lr_update_scheduler == 'Polynomial':
            p = text_frame.add_paragraph()
            p.text = 'decayed with polynomial learning rate decay (power = {}).'.format(
                lr_power)
            p.level = 3
        elif lr_update_scheduler == 'Step':
            if lr_steps != '':
                p = text_frame.add_paragraph()
                p.text = 'decayed at ' + lr_steps + ' ' + \
                    ('iteration' if lr_update_interval_unit else 'epoch') + ' using an exponential rate of ' + str(lr_multiplier) + ' .'
                p.level = 3
        if lr_warmup_scheduler != 'None' and lr_warmup_length > 0:
            p = text_frame.add_paragraph()
            p.text = 'warmed up linearly over the first {} {}.'.format(
                lr_warmup_length, 'iterations' if range_unit else 'epochs')
            p.level = 3

        # solver : other parameters
        num_solver_parameter = int(optimizer_config['SolverParameterNum'])
        for ip in range(num_solver_parameter - 1):
            p = text_frame.add_paragraph()
            p.text = optimizer_config['SolverParameterName_' + str(
                ip + 1)] + ' : ' + optimizer_config['SolverParameter_' + str(ip + 1)]
            p.level = 2

        # weight decay
        weight_decay = float(optimizer_config['WeightDecay'])
        p = text_frame.add_paragraph()
        p.text = 'Weight decay ' + \
            (': ' + str(weight_decay) if weight_decay != 0 else 'is not applied.')
        p.level = 1


def experimental_result_report(args, config, prs):
    if 'Result' not in config:
        return

    if 'LearningCurve_Image' in config['Result']:
        slide_layout = prs.slide_layouts[5]  # layout only with title
        slide = prs.slides.add_slide(slide_layout)

        # slide title
        slide.shapes.title.text = 'Experimental Result : Learning Curve'

        # learning curve image
        img_path = os.path.join(
            os.path.dirname(
                args.input),
            config['Result']['LearningCurve_Image'])
        ppc = 480 / 10.06
        im = imread(img_path)
        xscale = 22.86 * ppc / im.shape[1]
        yscale = 10.06 * ppc / im.shape[0]
        scale = xscale if xscale < yscale else yscale
        if scale > 1.0:
            scale = 1.0
        slide.shapes.add_picture(
            img_path,
            Cm(1.27),
            Cm(2.7),
            width=Cm(
                scale *
                im.shape[1] /
                ppc),
            height=Cm(
                scale *
                im.shape[0] /
                ppc))

    num_confusion_matrix = int(config['Result']['NumConfusionMatrix'])
    if num_confusion_matrix:
        slide_layout = prs.slide_layouts[1]  # layout with single placeholder
        slide = prs.slides.add_slide(slide_layout)

        # slide title
        slide.shapes.title.text = 'Experimental Result : Evaluation'

        # executor
        num_executor = int(config['Config']['NumExecutor'])
        text_frame = slide.shapes.placeholders[1].text_frame
        for i in range(num_executor):
            executor_config = config['Executor_' + str(i)]
            text = 'Evaluate network "' + \
                executor_config['Executor_NetworkName'] + '" using "' + executor_config['Executor_DatasetName'] + '" dataset.'
            if i == 0:
                text_frame.text = text
            else:
                p = text_frame.add_paragraph()
                p.text = text
                p.level = 0

            num_evaluation = int(executor_config['NumEvaluations'])
            if num_evaluation > 1:
                repeat_evaluation_type = int(
                    executor_config['RepeatEvaluationType'])
                with_back_propagation = int(
                    executor_config['NeedBackPropagation'])
                p = text_frame.add_paragraph()
                p.text = 'The calculation is executed ' + str(num_evaluation) + ' times for each data' + (
                    ' with back propagation' if with_back_propagation else '.')
                p.level = 1

                p = text_frame.add_paragraph()
                p.text = 'The ' + \
                    ('last' if repeat_evaluation_type else 'average') + ' value is used as a final output.'
                p.level = 1

        for i in range(num_confusion_matrix):
            confusion_matrix_file_name = config['Result']['ConfusionMatrix_' + str(
                i)]
            confusion_matrix_path = os.path.join(
                os.path.dirname(args.input), confusion_matrix_file_name)

            variable_name = os.path.split(confusion_matrix_path)[1].replace(
                'confusion_matrix_', '').replace('.csv', '')
            p = text_frame.add_paragraph()
            p.text = 'Variable : ' + variable_name
            p.level = 0
            with open(confusion_matrix_path, 'r', encoding='utf-8-sig') as f:
                reader = csv.reader(f)

                rows = [row for row in reader]
                for i in range(4):
                    row = rows[len(rows) - 4 + i]
                    p = text_frame.add_paragraph()
                    p.text = row[0] + ' : ' + row[1]
                    p.level = 1


def references(args, config, prs):
    slide_layout = prs.slide_layouts[1]  # layout with single placeholder
    slide = prs.slides.add_slide(slide_layout)

    slide.shapes.title.text = 'References'
    text_frame = slide.shapes.placeholders[1].text_frame

    p = text_frame.paragraphs[0]
    run = p.add_run()
    run.text = 'Sony Corporation. Neural Network Console : Not just train and evaluate. You can design neural networks with fast and intuitive GUI. https://github.com/sony/neural-network-console'
    run.font.size = Pt(16)

    p = text_frame.add_paragraph()
    run = p.add_run()
    run.text = 'Sony Corporation. Neural Network Libraries : An open source software to make research, development and implementation of neural network more efficient. https://github.com/sony/nnabla'
    run.font.size = Pt(16)

    num_reference = int(config['Reference']['NumReference'])
    for i in range(num_reference):
        p = text_frame.add_paragraph()
        run = p.add_run()
        run.text = config['Reference']['ReferenceLayer_' + str(
            i)] + ' - ' + config['Reference']['Reference_' + str(i)].replace('\\n', ',\t')
        run.font.size = Pt(16)


def create_pptx_report(args):
    # open config
    logger.log(99, 'Loading config file...')
    config = configparser.ConfigParser()
    config.read(args.input)

    # open template
    logger.log(99, 'Loading pptx template file...')
    prs = Presentation(args.template)

    # title slide
    logger.log(99, 'Creating title slide...')
    title_slide_layout = prs.slide_layouts[0]
    slide = prs.slides.add_slide(title_slide_layout)
    title = slide.shapes.title
    subtitle = slide.placeholders[1]

    title.text = config['Global']['ProjectName']
    subtitle.text = config['Description']['Text'].replace('\\n', '\n')

    # dataset report
    logger.log(99, 'Exporting dataset report...')
    dataset_report(args, config, prs)

    # network report
    logger.log(99, 'Exporting network report...')
    network_report(args, config, prs)

    # config report
    logger.log(99, 'Exporting configuration report...')
    config_report(args, config, prs)

    # experimental result report
    logger.log(99, 'Exporting experimental result...')
    experimental_result_report(args, config, prs)

    # references
    logger.log(99, 'Exporting references...')
    references(args, config, prs)

    # save presentation
    logger.log(99, 'Saving pptx file...')
    prs.save(args.output)
    logger.log(99, 'Export pptx completed successfully.')


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        '-i', '--input', help='path to report.ini', required=True)
    parser.add_argument(
        '-t', '--template', help='path template pptx file name', required=True)
    parser.add_argument(
        '-o', '--output', help='output pptx file name', required=True)
    parser.set_defaults(func=create_pptx_report)

    args = parser.parse_args()
    args.func(args)


if __name__ == '__main__':
    main()
