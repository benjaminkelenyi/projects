
import argparse
import os
import random
import sys
sys.path.append('/home/rambo/pvcnn')

import numba
import numpy as np

import open3d as o3d

sys.path.append(os.getcwd())

__all__ = ['predict']

PREDICT_ONE= False
PREDICT_SHAPE = None 
'''
    'Bag': [0],
    'Box': [1],
    'Cylinder': [2],
    'RbootFrame': [3],
'''

def prepare():
    from utils.common import get_save_path
    from utils.config import configs
    from utils.device import set_cuda_visible_devices

    # since PyTorch jams device selection, we have to parse args before import torch (issue #26790)
    parser = argparse.ArgumentParser()
    parser.add_argument('configs', nargs='+')
    parser.add_argument('--devices', default=None)
    args, opts = parser.parse_known_args()
    if args.devices is not None and args.devices != 'cpu':
        gpus = set_cuda_visible_devices(args.devices)
    else:
        gpus = []

    print(f'==> loading configs from {args.configs}')
    configs.update_from_modules(*args.configs)
    # define save path
    save_path = get_save_path(*args.configs, prefix='runs')
    os.makedirs(save_path, exist_ok=True)
    configs.train.save_path = save_path
    configs.train.checkpoint_path = os.path.join(save_path, 'latest.pth.tar')
    configs.train.best_checkpoint_path = os.path.join(save_path, 'best.pth.tar')

    # override configs with args
    configs.update_from_arguments(*opts)
    if len(gpus) == 0:
        configs.device = 'cpu'
        configs.device_ids = []
    else:
        configs.device = 'cuda'
        configs.device_ids = gpus
    configs.dataset.split = configs.evaluate.dataset.split
    if 'best_checkpoint_path' not in configs.evaluate or configs.evaluate.best_checkpoint_path is None:
        if 'best_checkpoint_path' in configs.train and configs.train.best_checkpoint_path is not None:
            configs.evaluate.best_checkpoint_path = configs.train.best_checkpoint_path
        else:
            configs.evaluate.best_checkpoint_path = os.path.join(configs.train.save_path, 'best.pth.tar')
    assert configs.evaluate.best_checkpoint_path.endswith('.pth.tar')
    configs.evaluate.stats_path = configs.evaluate.best_checkpoint_path.replace('.pth.tar', '.eval.npy')

    return configs

def visual_o3d():
    # Generate some neat n times 3 matrix using a variant of sync function
    x = np.linspace(-3, 3, 401)
    mesh_x, mesh_y = np.meshgrid(x, x)
    z = np.sinc((np.power(mesh_x, 2) + np.power(mesh_y, 2)))
    z_norm = (z - z.min()) / (z.max() - z.min())
    xyz = np.zeros((np.size(mesh_x), 3))
    xyz[:, 0] = np.reshape(mesh_x, -1)
    xyz[:, 1] = np.reshape(mesh_y, -1)
    xyz[:, 2] = np.reshape(z_norm, -1)
    print('xyz')
    print(xyz)
    # add by nishi
    print('xyz.shape=',xyz.shape)
    print('type(xyz)=',type(xyz))
  
  
    pcd = o3d.geometry.PointCloud()
    pcd.points = o3d.utility.Vector3dVector(xyz)
    #o3d.io.write_point_cloud("../../test_data/sync.ply", pcd)
    
    o3d.visualization.draw_geometries([pcd])


def visual_o3dx3(b,trans=True,fs=6):
    print('b.shape=',b.shape)
    if trans:
      b = b.transpose(1, 0)
      print('b.shape=',b.shape)
    pcd = o3d.geometry.PointCloud()
    pcd.points = o3d.utility.Vector3dVector(b[:,:3])
    # pcd.colors = o3d.utility.Vector3dVector(b[:,3:6]/255)
    # pcd.colors = o3d.utility.Vector3dVector(b[:,3:6])
    if fs==9:
      pcd.normals = o3d.utility.Vector3dVector(b[:,6:9])
    o3d.visualization.draw_geometries([pcd])

def view_pcd_cls(p_cloud,cls):
  #print('cls=',int(cls))
  data=[]
  for dt in p_cloud:
    if dt[8] == cls:
      data.append(dt)
  
  point_cloud = np.array(data)
  #print('point_cloud.shape=',point_cloud.shape)
  return point_cloud

'''
  add by nishi
'''
def predict(configs=None):
    # load model 
    configs = prepare() if configs is None else configs

    import math
    import torch
    import torch.backends.cudnn as cudnn
    import torch.nn.functional as F
    from tqdm import tqdm

    from meters.shapenet import MeterShapeNet

    ###########
    # Prepare #
    ###########


    if configs.device == 'cuda':
        cudnn.benchmark = True
        # It enables benchmark mode in cudnn.
        # benchmark mode is good whenever your input sizes for your network do not vary. This way, cudnn will look for the optimal set of algorithms for that particular configuration (which takes some time). 
        # This usually leads to faster runtime.
        # But if your input sizes changes at each iteration, then cudnn will benchmark every time a new size appears, possibly leading to worse runtime performances
        if configs.get('deterministic', False):
            cudnn.deterministic = True
            cudnn.benchmark = False
    if ('seed' not in configs) or (configs.seed is None):
        configs.seed = torch.initial_seed() % (2 ** 32 - 1)
    seed = configs.seed
    random.seed(seed)
    np.random.seed(seed)
    torch.manual_seed(seed)

    print(f'\n==> configs')
    print(configs)

    #################################
    # Initialize DataLoaders, Model #
    #################################

    print(f'\n==> loading dataset "{configs.dataset}"')
    dataset = configs.dataset()[configs.dataset.split]
    meter = MeterShapeNet()

    print(f'\n==> creating model "{configs.model}"')
    model = configs.model()
    if configs.device == 'cuda':
        model = torch.nn.DataParallel(model)
    model = model.to(configs.device)

    if os.path.exists(configs.train.checkpoint_path):
        print(f'==> loading checkpoint "{configs.evaluate.best_checkpoint_path}"')
        checkpoint = torch.load(configs.evaluate.best_checkpoint_path)
        model.load_state_dict(checkpoint.pop('model'))
        del checkpoint
    else:
        return
      
    # add by nishi
    # make class_no to shapes_no convert table  -> class2shape_tbl
    if PREDICT_ONE:
      #num_shapes=configs.data.num_shapes
      #num_classes=configs.data.num_classes
      class2shape_tbl = [0 for i in range(configs.data.num_classes)]
      for i,v in enumerate(meter.shape_name_to_part_classes.values()):
        start_c = v[0]
        end_c = v[-1]
        class2shape_tbl[start_c:end_c+1] =[i  for _ in range(start_c,end_c+1)]
      
    model.eval()

    ##############
    # prediction #
    ##############
    stats = np.zeros((configs.data.num_shapes, 2))

    for shape_index, (file_path, shape_id) in enumerate(tqdm(dataset.file_paths, desc='eval', ncols=0)):
        data = np.loadtxt(file_path).astype(np.float32)   # original data --> (2704,7)
        if True:
          print('\n-------------------------------------')
          print('>>>file_path=',file_path)
          print('data.shape=',data.shape)     # data.shape= (2749, 7) -> (p,(x,y,z,r,g,b,class_id))
          print('shape_id=',shape_id)         # shape_id = class id,  4 = chair
          visual_o3dx3(data,trans=False)
          #sys.exit()
          
        total_num_points_in_shape = data.shape[0]
        confidences = np.zeros(total_num_points_in_shape, dtype=np.float32)
        predictions = np.full(total_num_points_in_shape, -1, dtype=np.int64)

        coords = data[:, :3]    # coords = (p,x,y,z)  -> (p,C)
        if dataset.normalize:
            coords = dataset.normalize_point_cloud(coords)
        coords = coords.transpose()   # (p,C)  -> (C,p)  channel last -> channel first
        ground_truth = data[:, -1].astype(np.int64)   # original data (2704,6) -> (2706,) = class_id[2706]
        print("ground_truth->", ground_truth)
        if dataset.with_normal:
            normal = data[:, 3:6].transpose()   # (p,(r,g,b))  -> ((r,g,b),p)  channel first
            if dataset.with_one_hot_shape_id:
                shape_one_hot = np.zeros((dataset.num_shapes, coords.shape[-1]), dtype=np.float32)
                shape_one_hot[shape_id, :] = 1.0
                point_set = np.concatenate([coords, normal, shape_one_hot])
            else:
                point_set = np.concatenate([coords, normal])
        else:
            if dataset.with_one_hot_shape_id:
                shape_one_hot = np.zeros((dataset.num_shapes, coords.shape[-1]), dtype=np.float32)
                shape_one_hot[shape_id, :] = 1.0
                point_set = np.concatenate([coords, shape_one_hot]) # (x,y,z,shape_one_hot[16],p) -> (19,p)
            else:
                point_set = coords
        
        if PREDICT_ONE:
          #test by nishi
          extra_batch_size=1
        else:
          extra_batch_size = configs.evaluate.num_votes * math.ceil(total_num_points_in_shape / dataset.num_points)
          extra_batch_size = 8
        
        total_num_voted_points = extra_batch_size * dataset.num_points
        num_repeats = math.ceil(total_num_voted_points / total_num_points_in_shape)
        shuffled_point_indices = np.tile(np.arange(total_num_points_in_shape), num_repeats)
        shuffled_point_indices = shuffled_point_indices[:total_num_voted_points]
        np.random.shuffle(shuffled_point_indices)
        
        if False:
          print(' meter.part_class_to_shape_part_classes=', meter.part_class_to_shape_part_classes)
        
        start_class, end_class = meter.part_class_to_shape_part_classes[ground_truth[0]]

        
        if True:
          print('ground_truth.shape=',ground_truth.shape)
          print('ground_truth[0:5]=',ground_truth[0:5])
          print('start_class=',start_class, ' ,end_class=',end_class)
          print('\n>>point_set.shape=',point_set.shape)   # (22,2704)  -> ((x,y,z,r,g,b,shape_one_hot[16]),p)
          print('dataset.num_points=',dataset.num_points) # 2048
          
          #sys.exit()
          
        # model inference
        inputs = torch.from_numpy(
            point_set[:, shuffled_point_indices].reshape(-1, extra_batch_size, dataset.num_points).transpose(1, 0, 2)
        ).float().to(configs.device)
        
        if PREDICT_ONE:
          inputs_np = inputs.to('cpu').detach().numpy().copy()
          if True:
            print('\n>>inputs_np.shape=',inputs_np.shape)   # inputs_np.shape= (1, 22, 2048)   (batch,channel,p)
            print('extra_batch_size=',extra_batch_size)     # extra_batch_size= 1
            print('type(inputs_np)=',type(inputs_np))
          visual_o3dx3(inputs_np[0])
          #sys.exit()
        
        with torch.no_grad():
            vote_confidences = F.softmax(model(inputs), dim=1)
            if True:
              vote_confidences_np = vote_confidences.cpu().numpy()
              print('vote_confidences_np.shape=',vote_confidences_np.shape)
              # vote_confidences_np.shape= (20, 50, 2048)
              vote_confidences_x, vote_predictions_x = vote_confidences.max(dim=1)
              vote_confidences_x_np = vote_confidences_x.cpu().numpy()
              print('vote_confidences_x_np.shape=',vote_confidences_x_np.shape)
              print('vote_confidences_x_np[0,0:5]=',vote_confidences_x_np[0,0:5])
              
              vote_predictions_x_np = vote_predictions_x.cpu().numpy()
              print('vote_predictions_x_np.shape=',vote_predictions_x_np.shape)
              print('vote_predictions_x_np[0,0:5]=',vote_predictions_x_np[0,0:5])
            
            if PREDICT_ONE:
              # add by nishi  
              vote_confidences_x, vote_predictions_x = vote_confidences.max(dim=1)
              vote_confidences_x_np = vote_confidences_x.cpu().numpy()
              vote_predictions_x_np = vote_predictions_x.cpu().numpy()
            else:
              vote_confidences, vote_predictions = vote_confidences[:, start_class:end_class, :].max(dim=1)
              vote_confidences = vote_confidences.view(total_num_voted_points).cpu().numpy()
              vote_predictions = (vote_predictions + start_class).view(total_num_voted_points).cpu().numpy()

        if PREDICT_ONE:
          # add by nishi
          proc_predict(shape_index,vote_predictions_x_np,vote_confidences_x_np,inputs_np,
                       class2shape_tbl,predict_shape=PREDICT_SHAPE)
        else:
          update_shape_predictions(vote_confidences, vote_predictions, shuffled_point_indices,
                                   confidences, predictions, total_num_voted_points)
          # ground_truth[] -> input points
          # predictions[]  -> predict points
          update_stats(stats, ground_truth, predictions, shape_id, start_class, end_class)
       
        if True:
          print('\nshape_index=',shape_index)
          print('shape_id=',shape_id)
          #if shape_index >=10:
          #  sys.exit()
        
    print('>>>end')
    if not PREDICT_ONE:
      print('configs.evaluate.stats_path=',configs.evaluate.stats_path)
      # --> configs.evaluate.stats_path= runs\shapenet.pvcnn.c1\best.eval.npy
      np.save(configs.evaluate.stats_path, stats)
      print('clssIoU: {}'.format('  '.join(map('{:>8.2f}'.format, stats[:, 0] / stats[:, 1] * 100))))
      print('meanIoU: {:4.2f}'.format(stats[:, 0].sum() / stats[:, 1].sum() * 100))


def proc_predict(shape_index,vote_predictions_x_np,vote_confidences_x_np,inputs_np,class2shape_tbl,predict_shape=None):
  print('proc_predict()')
  vote_predictions_x_np=np.squeeze(vote_predictions_x_np)
  vote_confidences_x_np=np.squeeze(vote_confidences_x_np)
  print('vote_predictions_x_np.shape=',vote_predictions_x_np.shape)
  print('vote_predictions_x_np[0:5]=',vote_predictions_x_np[0:5])
  inputs_np=np.squeeze(inputs_np)
  inputs_np=inputs_np.transpose()
  print('inputs_np.shape=',inputs_np.shape)
  
  points=inputs_np.shape[0]
  out=[]
  for i in range(points):
    id=int(vote_predictions_x_np[i])
    inputs_np[i][6]=vote_confidences_x_np[i]    # acc
    inputs_np[i][7]=float(id)   # class id
    inputs_np[i][8]=float(class2shape_tbl[id])    # shape id
    out.append(inputs_np[i][:9])

  p_cloud = np.array(out)
  # p_cloud = point,c[9]  
  #            c =  x,y,z,r,g,b,acc,class_id,shape_id
  print('p_cloud.shape=',p_cloud.shape)
  #print('>>p_cloud[0]=',p_cloud[0])
  
  if predict_shape == None:
    visual_o3dx3(p_cloud,trans=False,fs=6)
  else:
    b=view_pcd_cls(p_cloud,predict_shape)
    if b.shape[0] > 0:
      visual_o3dx3(b,trans=False,fs=6)

@numba.jit()
def update_shape_predictions(vote_confidences, vote_predictions, shuffled_point_indices,
                             shape_confidences, shape_predictions, total_num_voted_points):
    # total_num_voted_points(40960) = dataset.num_points(2048) * class_no(20)
    for p in range(total_num_voted_points):
        point_index = shuffled_point_indices[p]
        current_confidence = vote_confidences[p]
        if current_confidence > shape_confidences[point_index]:
            shape_confidences[point_index] = current_confidence
            shape_predictions[point_index] = vote_predictions[p]

@numba.jit()
def update_stats(stats, ground_truth, predictions, shape_id, start_class, end_class):
    iou = 0.0
    for i in range(start_class, end_class):
        igt = (ground_truth == i)
        ipd = (predictions == i)
        union = np.sum(igt | ipd)
        intersection = np.sum(igt & ipd)
        if union == 0:
            iou += 1
        else:
            iou += intersection / union
    iou /= (end_class - start_class)
    # shape_id -> class id  by nishi

    stats[shape_id][0] += iou
    stats[shape_id][1] += 1

if __name__ == '__main__':
    predict()
