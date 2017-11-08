# coding: utf-8
require_relative 'nekosdk_advscript2'

require_relative 'converter_common'

class Nekosdk2StoryConverter
  include ImplicitChars
  include DetectQuotes

  POS_XR = [0.5, 0.3, 0.7, 0.1, 0.9, 0.2, 0.8]

  def initialize(fn, meta, lang, imgs_meta)
    @meta = meta
    @lang = lang
    @imgs_meta = imgs_meta

    @scr = NekosdkAdvscript2.from_file(fn)
    @id_to_idx = []
    @scr.nodes.each_with_index { |n, idx|
      @id_to_idx[n.id] = idx
    }

    @out = []
    @imgs = {}
    @chars = {}

    @layers = {}

    @pos_x = POS_XR.map { |x| (@meta['resolution']['w'] * x).to_i }
  end

  def out
    {
      'meta' => @meta,
      'imgs' => @imgs,
      'chars' => @chars,
      'script' => @out,
    }
  end

  def run
    i = 0
    loop {
      n = @scr.nodes[i]
      process_node(n)
      i = @id_to_idx[n.next_id]
      return if i.nil?
    }
  end

  def process_node(n)
    case n.opcode
    when 2
      process_wait(n)
    when 5
      process_text_display(n)
    when 10 # [背景ロード]
      process_bg_load(n)
    when 11 # [ＣＧロード]
      process_cg_load(n)
    when 13
      process_sprite_load(n)
    when 14 # [立ち絵削除] 位置:0
      process_sprite_delete(n)
    when 21 # [立ち絵全て削除]
      process_sprites_delete_all(n)
    when 30 # [ＢＧＭ再生]
      process_bgm_start(n)
    when 31 # [ＢＧＭ停止]
      process_bgm_stop(n)
    else
      puts "#{n.id}. type1=#{n.type1} opcode=#{n.opcode} some_ofs=#{n.some_ofs} next_id=#{n.next_id}"
      n.strs.each_with_index { |s, i|
        ss = s.value.rstrip
        puts "  s#{i}: #{ss.encode('UTF-8')}" unless ss.empty?
      }
      puts
    end
  end

  def process_text_display(n)
    s = strs(n)
    #cmd = s[0]
    char = s[1]
    txt = s[2]
    voice_fn = s[3].gsub(/\\/, '/')

    h = {
      'txt' => {@lang => txt},
    }

    if char.empty?
      h['op'] = 'narrate'
    else
      op, msg = detect_quotes(txt, true)
      h['op'] = op
      h['txt'] = {@lang => msg}
      h['char'] = get_char_by_name(char)
    end
    h['voice'] = voice_fn unless voice_fn.empty?

    @out << h

    @out << {'op' => 'keypress'}
  end

  # [ＢＧＭ再生] bgm\M01-m.ogg
  # ch:0/vol:40/pos:0/tm:3500
  def process_bgm_start(n)
    s = strs(n)
    params = parse_cmd(s[0], '[ＢＧＭ再生]')

    # TODO: vol, pos, tm
    @out << {
      'op' => 'sound_play',
      'fn' => convert_fn(s[1]),
      'channel' => "music#{params['ch']}",
      'loop' => true,
    }
  end

  # [ＢＧＭ停止]
  # ch:1 / tm:2500
  def process_bgm_stop(n)
    s = strs(n)
    params = parse_cmd(s[0], '[ＢＧＭ停止]')

    # TODO: support "tm"
    @out << {
      'op' => 'sound_stop',
      'channel' => "music#{params['ch']}",
    }
  end

  # [背景ロード]
  def process_bg_load(n)
    s = strs(n)
    @out << {
      'op' => 'img',
      'layer' => 'bg',
      'fn' => convert_fn(s[1])
    }
  end

  # [ＣＧロード]
  def process_cg_load(n)
    process_bg_load(n)
  end

  # [ウエイト]
  # time:3500
  def process_wait(n)
    s = strs(n)
    params = parse_cmd(s[0], '[ウエイト]')
    @out << {
      'op' => 'wait',
      't' => params['time'].to_i,
    }
  end

  # [立ち絵ロード] 位置:1
  # sc\a711_000.bmp
  def process_sprite_load(n)
    s = strs(n)
    params = parse_cmd(s[0], '[立ち絵ロード]')

    spr_file = s[1]
    spr_pos = params['位置'].to_i
    spr_fn = convert_fn(spr_file)

    img_meta = @imgs_meta[spr_fn]
    raise "no img meta for #{spr_fn.inspect}, got #{@imgs_meta.keys.inspect}" unless img_meta

    raise "no pos_x for spr_pos=#{spr_pos}" unless @pos_x[spr_pos]
    x = @pos_x[spr_pos] - img_meta[:w] / 2

    @out << {
      'op' => 'img',
      'layer' => "spr#{spr_pos}",
      'x' => x,
      'y' => 0,
      'fn' => spr_fn,
    }

    @layers[spr_pos] = true
  end

  # [立ち絵全て削除]
  def process_sprites_delete_all(n)
    @layers.each_key { |spr_pos|
      @out << {
        'op' => 'img',
        'layer' => "spr#{spr_pos}",
        'fn' => '',
      }
    }
    @layers = {}
  end

  # [立ち絵削除] 位置:0
  def process_sprite_delete(n)
    s = strs(n)
    params = parse_cmd(s[0], '[立ち絵削除]')
    spr_pos = params['位置'].to_i

    @out << {
      'op' => 'img',
      'layer' => "spr#{spr_pos}",
      'fn' => '',
    }
    @layers.delete(spr_pos)
  end

  def strs(n)
    n.strs.map { |s| s.value.rstrip.encode('UTF-8') }
  end

  def parse_cmd(cmd, expect_keyword)
    lines = cmd.split(/\n/)
    raise "invalid line 0: #{lines[0].inspect}" unless lines[0][0, expect_keyword.length] == expect_keyword

    rh = {}
    ra = []

    parse_cmdline(rh, ra, lines[0][expect_keyword.length..-1])
    lines[1..-1].each { |line|
      parse_cmdline(rh, ra, line)
    }

    rh[:pos] = ra

    return rh
  end

  def parse_cmdline(h, a, line)
    line.split(/\//).each { |kv_pair|
      kv = kv_pair.strip.split(/:/)
      if kv.size == 2
        k, v = kv
        h[k] = v
      else
        a << kv
      end
    }
  end

  def convert_fn(fn)
    fn.gsub(/\\/, '/').downcase.gsub(/\.bmp$/, '.png')
  end
end
