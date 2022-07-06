import torch

__all__ = ['MeterShapeNet']


default_shape_name_to_part_classes = {
    'Bag': [0],
    'Box': [1],
    'Cylinder': [2],
    'RobotFrame': [3],
}


class MeterShapeNet:
    def __init__(self, num_classes=4, num_shapes=4, shape_name_to_part_classes=None):
        super().__init__()
        self.num_classes = num_classes
        self.num_shapes = num_shapes

        self.shape_name_to_part_classes = default_shape_name_to_part_classes if shape_name_to_part_classes is None \
            else shape_name_to_part_classes
        part_class_to_shape_part_classes = []
        for shape_name, shape_part_classes in self.shape_name_to_part_classes.items():
            start_class, end_class = shape_part_classes[0], shape_part_classes[-1] + 1
            for _ in range(start_class, end_class):
                part_class_to_shape_part_classes.append((start_class, end_class))
        self.part_class_to_shape_part_classes = part_class_to_shape_part_classes
        self.reset()

    def reset(self):
        self.iou_sum = 0
        self.shape_count = 0

    def update(self, outputs: torch.Tensor, targets: torch.Tensor):
        # outputs: B x num_classes x num_points, targets: B x num_points
        for b in range(outputs.size(0)):
            start_class, end_class = self.part_class_to_shape_part_classes[targets[b, 0].item()]
            prediction = torch.argmax(outputs[b, start_class:end_class, :], dim=0) + start_class
            target = targets[b, :]
            iou = 0.0
            for i in range(start_class, end_class):
                itarget = (target == i)
                iprediction = (prediction == i)
                union = torch.sum(itarget | iprediction).item()
                intersection = torch.sum(itarget & iprediction).item()
                if union == 0:
                    iou += 1.0
                else:
                    iou += intersection / union
            iou /= (end_class - start_class)
            self.iou_sum += iou
            self.shape_count += 1

    def compute(self):
        return self.iou_sum / self.shape_count
