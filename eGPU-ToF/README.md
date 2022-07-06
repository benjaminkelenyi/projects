
# [Embedded  GPU  based  ToF  image  recognition  for  mobile robots](http://embeddedgputof.tk/)

Created a novel architecture based on PV-CNN that combines  depth  imaging  with  IR.   This  architecture  was  designed particularly for pulse-based Time of Flight (ToF) cameras and the  primary  algorithm’s  target  being  embedded  devices.  Wetested  the  proposed  algorithm  on  custom  indoor/outdoor andpublic datasets,  using  different  camera  vendors.

![arch](https://user-images.githubusercontent.com/22835687/141301762-c58fdb51-1fca-4918-9092-9be9d13f6147.PNG)

## Prerequisites
```
conda env create -f configs/env_eGPU-ToF.yml
```
## Training
```
python train.py [config-file] --devices [gpu-ids]
```

### Pretrained Models
You can find our pretrained models [here](https://mega.nz/file/ef5wVALC#Co67nmqBumSp6YwOnGZZcrTZAxSBbgZAO_4dgewIAFU).

## Evaluation
```
python train.py [config-file] --devices [gpu-ids] --evaluate
```

## Our Results
![results](https://user-images.githubusercontent.com/22835687/141303251-51d168c5-49c2-4dc8-a255-96c75cfcbdd9.PNG)


## Acknowledgements

The authors are thankful for the support of Andrei Cozma through Analog Devices GMBH Romania, for the equipmentlist  (cameras,  embedded  devices,  GPUs)  offered  as  supportto this work. This work was financially supported by the Romanian  National  Authority  for  Scientific  Research,  CNCS-UEFISCDI, project number PN-III-P2-2.1-PTE-2019-0367.
