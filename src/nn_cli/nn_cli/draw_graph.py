# Copyright 2019,2020,2021 Sony Corporation.
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


def draw_graph_command(args):
    import os
    from nnabla.logger import logger
    from nnabla.utils import nnp_graph
    import nnabla.functions as F
    from nnabla.experimental.viewers import SimpleGraph

    logger.info('Loading: %s' % args.input)
    nnp = nnp_graph.NnpLoader(args.input)
    names = nnp.get_network_names()
    logger.info('Available networks: %s' % repr(names))

    if not args.network:
        logger.info('Drawing all networks as `--network` option is not passed.')
        draw_networks = names
    else:
        draw_networks = []
        for n in args.network:
            assert n in names, "Specified a network `%s` that doesn't exist." % n
            draw_networks.append(n)

    if not os.path.isdir(args.output_dir):
        os.makedirs(args.output_dir)

    logger.info("Save drawn network(s) into `%s`." % args.output_dir)

    for n in draw_networks:
        logger.info("Drawing: %s" % n)
        graph = nnp.get_network(n)
        if not graph.outputs:
            logger.info("No output in `%s`. Skipping." % n)
            continue
        elif len(graph.outputs) > 1:
            out = F.sink(*graph.outputs.values())
        else:
            out = list(graph.outputs.values())[0]

        sg = SimpleGraph(format=args.format)
        sg.save(out, os.path.join(args.output_dir,
                                  n.replace(' ', '_')), cleanup=True)

    return True



