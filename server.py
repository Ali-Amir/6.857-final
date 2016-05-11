import pika
from collections import defaultdict
import time
from collections import deque

LATEST_TS = 0.0
LAPTOP_DISTANCE = 2.5
PIXEL_DISTANCE = 200
WIDTH = 1024
HEIGHT = 800
TIME_THRESH = 4000.0
alpha = 0.9

class TimeAverage:

  def __init__(self):
    self.mapping = defaultdict(deque)
    self.tot_sum = defaultdict(float)

  def relax(self, key):
    while len(self.mapping[key]) > 1:
      val, ts = self.mapping[key].popleft()
      if LATEST_TS - ts > TIME_THRESH:
        self.tot_sum[key] -= val
      else:
        self.mapping[key].appendleft((val, ts))
        break
    return

  def add(self, mac, value, ts):
    self.mapping[mac].append((value, ts))
    self.tot_sum[mac] += value
    self.relax(mac)
    return

  def get(self, mac):
    return self.tot_sum[mac] / len(self.mapping[mac])

R = [TimeAverage(), TimeAverage()]
Xmap = TimeAverage()
Ymap = TimeAverage()





def rolling_update(dic, key, new_val, alpha):
  if not key in dic:
    dic[key] = new_val
  else:
    dic[key] = dic[key] * (1.0 - alpha) + new_val * alpha

def update(rid, mac, dis):
  rolling_update(R[rid], mac, dis, alpha)

######################
###### GEOMETRY ######
######################
def intersect(r0, r1):
  add = max(0.0, (LAPTOP_DISTANCE - r0 - r1)/2.0)
  r0 += add
  r1 += add

  dx = (max(0.0, r0**2.0 - ((r1**2.0 - r0**2.0 - LAPTOP_DISTANCE**2.0)**2.0) / ((2*LAPTOP_DISTANCE)**2.0)))**0.5

  dy = LAPTOP_DISTANCE/2.0 - ((r1**2.0 - dx**2.0)**0.5)
  return dx, dy

######################
######## UI ##########
######################
from Tkinter import *
root = Tk()
def drawcircle(canv,x,y,rad,color='blue'):
    canv.create_oval(x-rad,y-rad,x+rad,y+rad,width=0,fill=color)

canvas = Canvas(width=WIDTH, height=HEIGHT, bg='white')
canvas.pack(expand=YES, fill=BOTH)
def ui_update(ts):
  y0 = HEIGHT / 2 - PIXEL_DISTANCE / 2
  x0 = 40
  y1 = HEIGHT / 2 + PIXEL_DISTANCE / 2
  x1 = 40
  global Xmap
  global Ymap

  canvas.delete("all")
  circ1=drawcircle(canvas,x0, y0, 20)
  circ2=drawcircle(canvas,x1, y1, 20)
  for mac in R[0].tot_sum:
    if not mac in R[1].tot_sum:
      continue
    x, y = intersect(R[0].get(mac), R[1].get(mac))
    Xmap.add(mac, x, ts)
    Ymap.add(mac, y, ts)
    #rolling_update(Xmap, mac, x, alpha)
    #rolling_update(Ymap, mac, y, alpha)
    x, y = (Xmap.get(mac), Ymap.get(mac))
    x_cen = int(x * PIXEL_DISTANCE / LAPTOP_DISTANCE + 40)
    y_cen = int(y * PIXEL_DISTANCE / LAPTOP_DISTANCE + HEIGHT / 2)
    print x_cen, y_cen
    color = 'red'
    if mac[0] == 'd':
      color = 'green'
    drawcircle(canvas, x_cen, y_cen, 30, color)
    text = canvas.create_text(x_cen,y_cen, text=mac)

  root.update()

######################
######## RABBIT_MQ ##########
######################
credentials = pika.PlainCredentials('a', 'a')
connection = pika.BlockingConnection(pika.ConnectionParameters(
        host='127.0.0.1', credentials = credentials))
channel = connection.channel()
args = {"x-max-length":1}
channel.queue_delete(queue='6.857-0')
channel.queue_delete(queue='6.857-1')
channel.queue_declare(queue='6.857-0', durable=True, arguments=args)
channel.queue_declare(queue='6.857-1', durable=True, arguments=args)
asd

def callback(ch, method, properties, body):
    #print " [x] Received %r" % (body,)
    rid, mac, dis, ts = body.split('|')
    rid = int(rid)
    dis = float(dis)
    ts = float(ts)
    global LATEST_TS
    LATEST_TS = max(LATEST_TS, ts)
    R[rid].add(mac, dis, ts)
    #update(rid, mac, dis, ts)
    ui_update(ts)

channel.basic_consume(callback,
                      queue="6.857",
                      no_ack=True)
print "here!"
channel.start_consuming()
