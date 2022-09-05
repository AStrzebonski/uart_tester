library ieee;
use ieee.std_logic_1164.all;


entity reg32 is
  generic (
    DWIDTH : integer := 32;
    AWIDTH : integer := 2;
    DBYTES : integer := 4
  );
  port (
    clk          : in std_logic;
    resetn      : in std_logic;
    read        : in std_logic;
    write       : in std_logic;
    readdata    : out std_logic_vector(DWIDTH-1 downto 0);
    writedata   : in std_logic_vector(DWIDTH-1 downto 0);
    addr        : in std_logic_vector(AWIDTH-1 downto 0);
    tx_o         : out std_logic;
    rx_i         : in std_logic;
    Q            : out std_logic_vector(DWIDTH-1 downto 0)
  );
 end reg32;

architecture rtl of reg32 is

  -- COMPONENTS
  component prmcu_uart_top is
  port(
    clk                    : in  std_logic;
    rst                    : in  std_logic;

    -- control
    uart_en                : in  std_logic;
    tx_en                  : in  std_logic;
    rx_en                  : in  std_logic;
    n_parity_bits_i        : in  std_logic;
    n_stop_bits_i          : in  std_logic_vector(1 downto 0);
    n_data_bits_i          : in  std_logic_vector(3 downto 0);
    internal_clk_divider_i : in  std_logic_vector(15 downto 0);

    --input axi interface
    in_dat_i               : in  std_logic_vector(8 downto 0);
    in_vld_i               : in  std_logic;
    in_rdy_o               : out std_logic;

    -- output axi interface
    out_dat_o              : out std_logic_vector(8 downto 0);
    out_vld_o              : out std_logic;
    out_rdy_i              : in  std_logic;

    --to external device
    tx_o                   : out std_logic;
    rx_i                   : in  std_logic
    );
  end component;

  signal ADDR_CONTROL_REG : std_logic_vector(AWIDTH-1 downto 0) := "00";
  signal ADDR_STATUS_REG : std_logic_vector(AWIDTH-1 downto 0) := "01";
  signal ADDR_TX_REG : std_logic_vector(AWIDTH-1 downto 0) := "10";
  signal ADDR_RX_REG : std_logic_vector(AWIDTH-1 downto 0) := "11";

  signal CONTROL_REG : std_logic_vector(DWIDTH-1 downto 0);
  signal STATUS_REG : std_logic_vector(DWIDTH-1 downto 0);
  signal TX_REG : std_logic_vector(DWIDTH-1 downto 0);
  signal RX_REG : std_logic_vector(DWIDTH-1 downto 0) := (others => '0');

  signal tx_vld_r : std_logic;
  signal tx_rdy : std_logic;
  signal rx_vld : std_logic;
  signal rx_rdy : std_logic;

  signal reset_uart_r : std_logic;
  signal readdata_r : std_logic_vector(DWIDTH-1 downto 0);
  signal read_r : std_logic;


--  function assign_bytewise(
--     input      : std_logic_vector;
--     output     : std_logic_vector;
--	 byteenable : std_logic_vector)
--	 return std_logic_vector is
--	 variable ret : std_logic_vector(DWIDTH-1 downto 0) := output;
--  begin
--
--	   for i in 0 to DBYTES-1 loop
--		  if byteenable(i) = '1' then
--		    ret((i+1)*8-1 downto i*8) := input((i+1)*8-1 downto i*8);
--		  end if;
--		end loop;
--	 return ret;
--
--  end function;

  function map_stop_bits(
    reg : std_logic)
    return std_logic_vector is
    variable n_stop_bits : std_logic_vector(1 downto 0);
  begin

    if reg = '0' then
      n_stop_bits := "01";
    else
      n_stop_bits := "10";
    end if;

    return n_stop_bits;
  end function;

begin

  -- INSTANCE
  uart_i : prmcu_uart_top
  port map(
    clk                    => clk,
    rst                    => (not resetn) or reset_uart_r,

    uart_en                => CONTROL_REG(0),
    tx_en                  => CONTROL_REG(1),
    rx_en                  => CONTROL_REG(2),

    n_parity_bits_i        => CONTROL_REG(8),
    n_stop_bits_i          => map_stop_bits(CONTROL_REG(9)),
    n_data_bits_i          => CONTROL_REG(13 downto 10),

    internal_clk_divider_i => CONTROL_REG(31 downto 16),

    in_dat_i               => TX_REG(8 downto 0),
    in_vld_i               => tx_vld_r,
    in_rdy_o               => tx_rdy,

    out_dat_o              => RX_REG(8 downto 0),
    out_vld_o              => rx_vld,
    out_rdy_i              => rx_rdy,

    tx_o                   => tx_o,
    rx_i                   => rx_i
  );

  --COMBINATIONAL LOGIC
  status_reg_comb_p : process(rx_vld) begin
    --STATUS REG
    STATUS_REG(7 downto 0) <= x"1A"; --SR(4 - vcc, 5 - gnd)
    STATUS_REG(30 downto 8) <= (others => '0');
    STATUS_REG(31) <= rx_vld;
  end process;

  readdata_reg_p : process(clk) begin
    -- READDATA
    if rising_edge(clk) then
      case addr is
        when "00" =>

          if (read = '1') then
            readdata_r <= CONTROL_REG;
          end if;

        when "01" =>
          if (read = '1') then
            readdata_r <= STATUS_REG;
          end if;

        when  "10" =>
          if (read = '1') then
            readdata_r <= TX_REG;
          end if;

        when others =>
          if (read = '1') then
            readdata_r <= RX_REG;
          end if;
      end case;
    end if;

  end process;

  read_r_p : process(clk) begin
    if rising_edge(clk) then
      read_r <= read;

      if (resetn = '0') then
        read_r <= '0';
      end if;

    end if;
  end process;

  rx_rdy_p : process(read, read_r, addr) begin
    if (read = '1' and read_r = '1' and addr = "11") then
      rx_rdy <= '1';
    else
      rx_rdy <= '0';
    end if;
  end process;


  reg_write_p : process(clk) begin
    if rising_edge(clk) then
    reset_uart_r <= '0';
    tx_vld_r <= '0';
    case addr is

      when  "00" => --CONTROL_REG
        if (write = '1') then
          CONTROL_REG <= writedata;
          reset_uart_r <= '1';
        end if;

      when "01" => --STATUS_REG

      when "10" => --TX_REG
        if (write = '1') then
          TX_REG <= writedata;
          tx_vld_r <= '1';
        end if;

      when others => --RX_REG

    end case;

   end if;
  end process;
  
  readdata <= readdata_r;
  Q <= STATUS_REG;


end rtl;