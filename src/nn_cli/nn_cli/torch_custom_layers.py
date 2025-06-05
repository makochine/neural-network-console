# Copyright 2024,2025 Sony Group Corporation.
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

import torch
import torch.nn as nn
import numpy as np


class Reshape(nn.Module):
    def __init__(self, new_shape):
        super(Reshape, self).__init__()
        self.new_shape = new_shape

    def forward(self, x):
        return x.reshape(*self.new_shape)


class GlobalAvgPoolNd(nn.Module):
    def __init__(self):
        super(GlobalAvgPoolNd, self).__init__()

    def forward(self, x):
        axes_to_reduce = tuple(range(2, len(x.shape)))  # Axes to reduce (excluding batch and channel dimensions)
        return torch.mean(x, dim=axes_to_reduce)


class SumPool(nn.Module):
    def __init__(self, kernel_size, stride=(1, 1), padding=(0, 0)):
        super(SumPool, self).__init__()
        if isinstance(padding, int):
            self.padding = (padding, padding)
        else:
            self.padding = padding
        if isinstance(stride, int):
            self.stride = (stride, stride)
        else:
            self.stride = stride
        self.kernel_size = kernel_size

    def forward(self, x):
        batch_size, channels, height, width = x.size()
        kh, kw = self.kernel_size
        sh, sw = self.stride
        ph, pw = self.padding
        x = nn.functional.pad(x, (pw, pw, ph, ph))
        unfolded = nn.functional.unfold(x, kernel_size=(kh, kw), stride=(sh, sw))
        unfolded = unfolded.view(batch_size, channels, kh * kw, -1, unfolded.size(-1))
        output = unfolded.sum(dim=2)
        h_out = (height + 2 * ph - kh) // sh + 1
        w_out = (width + 2 * pw - kw) // sw + 1
        output = output.view(batch_size, channels, h_out, w_out)

        return output

class Unpooling(nn.Module):
    def __init__(self, kernel_size):
        super(Unpooling, self).__init__()
        self.kernel_size = kernel_size

    def forward(self, x):
        batch_size, channels, height, width = x.size()
        kh, kw = self.kernel_size

        # Initialize the unpooled tensor with zeros
        unpooled = torch.zeros(batch_size, channels, height * kh, width * kw, device=x.device)

        # Iterate over each element in the pooled tensor
        for i in range(height):
            for j in range(width):
                h_start = i * kh
                w_start = j * kw
                h_end = h_start + kh
                w_end = w_start + kw
                # Distribute the pooled value back to its original position
                unpooled[:, :, h_start:h_end, w_start:w_end] = x[:, :, i, j].unsqueeze(2).unsqueeze(3)

        return unpooled


class CReLU(nn.Module):
    def __init__(self, axis=1):
        super(CReLU, self).__init__()
        self.axis = axis

    def forward(self, x):
        # Compute ReLU of x and -x
        relu_x = torch.relu(x)
        relu_minus_x = torch.relu(-x)

        # Concatenate along the specified axis
        output = torch.cat((relu_x, relu_minus_x), dim=self.axis)

        return output


class Cumsum(nn.Module):
    def __init__(self, dim=0, exclusive=False, reverse=False):
        super(Cumsum, self).__init__()
        self.dim = dim
        self.exclusive = exclusive
        self.reverse = reverse

    def forward(self, x):
        # Perform cumsum operation with additional arguments
        if self.reverse:
            x = torch.flip(x, dims=(self.dim,))

        cumsum = torch.cumsum(x, dim=self.dim)

        if self.exclusive:
            shifted_cumsum = torch.roll(cumsum, 1, dims=self.dim)
            if self.dim == 0:
                shifted_cumsum[0] = 0
            else:
                shifted_cumsum[(slice(None),) * self.dim + (0,)] = 0
            cumsum = shifted_cumsum

        if self.reverse:
            cumsum = torch.flip(cumsum, dims=(self.dim,))

        return cumsum


class Cumprod(nn.Module):
    def __init__(self, dim=0, exclusive=False, reverse=False):
        super(Cumprod, self).__init__()
        self.dim = dim
        self.exclusive = exclusive
        self.reverse = reverse

    def forward(self, x):
        if self.reverse:
            x = torch.flip(x, dims=(self.dim,))

        cumprod = torch.cumprod(x, dim=self.dim)

        if self.exclusive:
            shifted_cumprod = torch.roll(cumprod, 1, dims=self.dim)
            if self.dim == 0:
                shifted_cumprod[0] = 0
            else:
                shifted_cumprod[(slice(None),) * self.dim + (0,)] = 0
            cumprod = shifted_cumprod

        if self.reverse:
            cumprod = torch.flip(cumprod, dims=(self.dim,))

        return cumprod


class Bmm(nn.Module):
    def __init__(self):
        super(Bmm, self).__init__()

    def forward(self, input_tensor, side_connect_tensor):
        input_dims = input_tensor.dim()
        side_connect_dims = side_connect_tensor.dim()

        # Ensure both tensors have at least 3 dimensions
        assert input_dims >= 3 and side_connect_dims >= 3, "Both input tensors must have at least 3 dimensions"

        # Get the shape of the input tensors excluding the last two dimensions
        input_shape_except_last_two = input_tensor.shape[:-2]
        side_connect_shape_except_last_two = side_connect_tensor.shape[:-2]
        assert input_shape_except_last_two == side_connect_shape_except_last_two, "Shapes excluding the last two dimensions must match"
        input_reshaped = input_tensor.view(-1, *input_tensor.shape[-2:])
        side_connect_reshaped = side_connect_tensor.view(-1, *side_connect_tensor.shape[-2:])

        result = torch.bmm(input_reshaped, side_connect_reshaped)
        result_shape = input_shape_except_last_two + result.shape[1:]
        result = result.view(*result_shape)

        return result


class BatchInv(nn.Module):
    def __init__(self):
        super(BatchInv, self).__init__()

    def forward(self, input_tensor):
        input_dims = input_tensor.dim()
        assert input_dims >= 2, "Input tensor must have at least 2 dimensions"

        input_shape_except_last_two = input_tensor.shape[:-2]
        input_reshaped = input_tensor.view(-1, *input_tensor.shape[-2:])

        result = torch.linalg.inv(input_reshaped)
        result_shape = input_shape_except_last_two + result.shape[1:]
        result = result.view(*result_shape)
        return result


class Prune(torch.nn.Module):
    def __init__(self, rate):
        super(Prune, self).__init__()
        self.rate = rate

    def forward(self, x):
        threshold = np.sort(np.abs(x.cpu().numpy())).flatten()[int((x.numel() - 1) * self.rate)]
        x = torch.where(torch.abs(x) < threshold, torch.tensor(0.0, device=x.device), x)
        return x


class NMSDetection2D(nn.Module):
    def __init__(self, thresh=0.5, nms=0.45, nms_per_class=True):
        super(NMSDetection2D, self).__init__()
        self.thresh = thresh
        self.nms = nms
        self.nms_per_class = nms_per_class

    def forward(self, x):
        B, C, H, W = x.size()
        N = H * W

        x = x.permute(0, 2, 3, 1).contiguous().view(B, -1, C)

        boxes = x[:, :, :4]
        objectness = x[:, :, 4:5]
        class_probs = x[:, :, 5:]

        if self.nms_per_class:
            for cls in range(class_probs.size(2)):
                scores = objectness * class_probs[:, :, cls].unsqueeze(2)
                for i in range(N):
                    for j in range(i + 1, N):
                        if torch.sum(scores[i] * scores[j]) > 0:
                            overlap = self.compute_iou(boxes[i], boxes[j])
                            if overlap > self.nms:
                                scores[j] = 0
            x[:, :, 5:] = objectness * class_probs

        else:
            for i in range(N):
                for j in range(i + 1, N):
                    if torch.sum(objectness[i] * objectness[j]) > 0:
                        overlap = self.compute_iou(boxes[i], boxes[j])
                        if overlap > self.nms:
                            objectness[j] = 0
                            class_probs[j] = 0

            # Set class probabilities to 0 for suppressed boxes
            x[:, :, 5:] = torch.where(objectness > 0, objectness * class_probs, torch.tensor(0.))
        return x

    def compute_iou(self, box1, box2):
        # Calculate intersection area
        x1 = torch.max(box1[0], box2[0])
        y1 = torch.max(box1[1], box2[1])
        x2 = torch.min(box1[0] + box1[2], box2[0] + box2[2])
        y2 = torch.min(box1[1] + box1[3], box2[1] + box2[3])
        intersection = max(0, x2 - x1) * max(0, y2 - y1)

        # Calculate union area
        area1 = box1[2] * box1[3]
        area2 = box2[2] * box2[3]
        union = area1 + area2 - intersection

        # Calculate IoU
        iou = intersection / union
        return iou


class ElementWiseEpsilonInsensitiveLoss(nn.Module):
    def __init__(self, epsilon):
        super(ElementWiseEpsilonInsensitiveLoss, self).__init__()
        self.epsilon = epsilon

    def forward(self, output, target):
        error = torch.abs(output - target)
        loss = torch.max(torch.zeros_like(error), error - self.epsilon)
        return loss.mean()