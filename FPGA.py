# FPGA processing

class UH:
  def __init__(self):
    self.side = None
    self.id = -1
    self.det = -1
    self.energy = -1
    self.timing = -1
    self.has_timing = False
  def is_valid(self):
    # AZ: According to Brent, everything has timing now:
    #if self.has_timing and self.adc >= 82:
    if self.adc >= 82:
      return True
    else:
      return False
      
def check_hits(uhs):
  n = len(uhs)
  print("Length detector hits: {}".format(n))
  # AZ: This rejects all events which happen in neighboring pixels
  # I still think we can revover the depth in those, and  
  # events with large depth difference can be good Comptons
  #for i in range(n):
  #  print(uhs[i].id)
  #  for j in range(i+1,n):
  #    if uhs[i].is_valid() and uhs[j].is_valid():
  #      if abs(uhs[i].id - uhs[j].id) >= 2:
  #        return True
  ValidCounter = 0
  for i in range(n):
    if uhs[i].is_valid():
      ValidCounter += 1
  if (ValidCounter > 1):
    return True
  
  return False

def check_single(d):
  print("Length single event: {}".format(len(d)))
  # AZ: Commented this out since this throws out hits in mutiple detectors!
  #if len(d) != 1:
  #  return False
  
  # AZ: Everything with hits in more than one detector is automatically good
  if len(d) > 2:
    print("--> Passed: More than 1 detector hit")
    return True
  
  # AZ: Added proper initialization
  xgood = False
  ygood = False
  
  for det in d:
    xgood = check_hits(d[det]['x'])
    ygood = check_hits(d[det]['y'])
    if xgood or ygood:
      print("--> Passed: 2 or more strips on at least one detector side")
      return True
  print("--> Not passed")
  return False


f = open('FPGA_Input.txt')
count = 0
event = {}
for line in f:
  print(line.rstrip())
  if line.startswith('SE'):
    if check_single(event):
      count += 1
    event = {}
  elif line.startswith('UH'):
    uh = UH()
    sp = line.split()
    uh.det = int(sp[1])
    uh.side = sp[2]
    uh.id = int(sp[3])
    uh.adc = int(sp[4])
    uh.timing = int(sp[5])
    uh.has_timing = bool(int(sp[6]))
    if uh.det not in event:
      event[uh.det] = {'x':[],'y':[]}
    event[uh.det][uh.side].append(uh)

f.close()

print("Good counts: {}".format(count))



